// SPDX-FileCopyrightText: 2026 KiraFlux
// SPDX-License-Identifier: GPL-3.0-or-later
// KiraFlux Toolkit - https://github.com/KiraFlux/kiraflux-toolkit

/// @file    drv/NVM.hpp
/// @brief   Typed non-volatile memory driver with compile-time dispatch.

#pragma once

#include <cstring>
#include <type_traits>

#ifdef ARDUINO
#include <nvs.h>
#include <nvs_flash.h>
#endif

#include "kf/Bytes.hpp"
#include "kf/BytesView.hpp"
#include "kf/Option.hpp"
#include "kf/Result.hpp"
#include "kf/Sequence.hpp"
#include "kf/Slice.hpp"
#include "kf/StringView.hpp"
#include "kf/core.hpp"

#include "kf/drv/Driver.hpp"

namespace kf {

namespace internal {

enum struct NvmError {
    NoSlot,
    PartitionNotFound,
    NoFreePages,
    NewVersionFound,

    InvalidKey,
    KeyNotFound,
    TypeMismatch,
    BlobSizeMismatch,
    ValueTooLong,
    BufferTooSmall,
    NotEnoughSpace,

    Internal,
};

struct NvmConfig {
    static constexpr usize max_namespace_length{
#ifdef ARDUINO
        NVS_NS_NAME_MAX_SIZE
#else
        16
#endif
    };

    /// @note nvs_open accepts 1 to max_namespace_length - 1 characters.
    char nvm_namespace[max_namespace_length];
};

template<typename Impl> using NvmBase = drv::Driver<
    Impl,
    drv::DriverTraits<
        NvmConfig,
        NoneType,
        NvmError>>;

}// namespace internal

namespace drv {

template<typename T>
concept nvm_native_int =
    std::same_as<T, i8> or std::same_as<T, u8> or
    std::same_as<T, i16> or std::same_as<T, u16> or
    std::same_as<T, i32> or std::same_as<T, u32> or
    std::same_as<T, i64> or std::same_as<T, u64>;

template<typename T>
concept nvm_native_enum = enum_type<T> and nvm_native_int<std::underlying_type_t<T>>;

/// @note Excludes Sequences, pointers - trivially copyable,
///       but their blob representation is a dangling pointer.
template<typename T>
concept nvm_blob_pod =
    trivial<T> and std::is_standard_layout_v<T> and !nvm_native_int<T> and !nvm_native_enum<T> and !std::same_as<T, bool> and !implements<T, SequenceTag> and !std::is_pointer_v<T>;

template<typename T>
concept nvm_fixed = nvm_native_int<T> or nvm_native_enum<T> or std::same_as<T, bool> or nvm_blob_pod<T>;

/// @brief Typed non-volatile memory driver.
/// @note Key/value map over ESP-IDF NVS.
///       Fixed types dispatch to native NVS operations, PODs to blobs,
///       Bytes to variable-length blobs.
///       `set` does not commit - call `commit()` once after a batch.
struct NVM;

#ifdef ARDUINO

struct NVM : internal::NvmBase<NVM> {

private:
    struct Key {
        static constexpr usize max_key_length{NVS_KEY_NAME_MAX_SIZE};

        char buf[max_key_length]{};

        [[nodiscard]] static auto create(StringView key) noexcept -> Result<Key, Traits::Error> {
            if (key.empty() or key.length() >= max_key_length) {
                return error(Traits::Error::InvalidKey);
            }
            Key k;
            std::memcpy(k.buf, key.data(), key.length());
            k.buf[key.length()] = '\0';
            return ok(k);
        }
    };

public:
    using internal::NvmBase<NVM>::NvmBase;

    /// @brief Read a fixed value of T stored under key.
    template<nvm_fixed T> [[nodiscard]] auto get(StringView key) const noexcept -> Result<T, Traits::Error> {
        auto const k = kf_try(Key::create(key));

        esp_err_t e;
        T v{};

        if constexpr (nvm_native_int<T>) {
            e = nvs_get(_handle, k.buf, &v);
        } else if constexpr (nvm_native_enum<T>) {
            std::underlying_type_t<T> raw{};
            e = nvs_get(_handle, k.buf, &raw);
            if (ESP_OK == e) { v = static_cast<T>(raw); }
        } else if constexpr (std::same_as<T, bool>) {
            u8 raw{};
            e = nvs_get(_handle, k.buf, &raw);
            if (ESP_OK == e) { v = raw != 0; }
        } else {
            usize len = sizeof(T);
            e = nvs_get_blob(_handle, k.buf, &v, &len);
            if (ESP_ERR_NVS_INVALID_LENGTH == e or (ESP_OK == e and len != sizeof(T))) {
                return error(Traits::Error::BlobSizeMismatch);
            }
        }

        if (ESP_OK != e) { return map_nvs_error(e); }
        return ok(v);
    }

    /// @brief Write a fixed value of T under key.
    [[nodiscard]] auto set(StringView key, nvm_fixed auto const &value) noexcept -> Traits::ActionResult {
        auto const k = kf_try(Key::create(key));

        esp_err_t e;

        using T = std::remove_cvref_t<decltype(value)>;
        if constexpr (nvm_native_int<T>) {
            e = nvs_set(_handle, k.buf, value);
        } else if constexpr (nvm_native_enum<T>) {
            e = nvs_set(_handle, k.buf, static_cast<std::underlying_type_t<T>>(value));
        } else if constexpr (std::same_as<T, bool>) {
            e = nvs_set(_handle, k.buf, static_cast<u8>(value));
        } else {
            e = nvs_set_blob(_handle, k.buf, &value, sizeof(T));
        }

        return map_nvs_result(e);
    }

    /// @brief Read a variable-length blob into buffer.
    /// @note Returns a view over actually read bytes - a shorter buffer
    ///       is not an error.
    [[nodiscard]] auto get(StringView key, Bytes buffer) const noexcept -> Result<BytesView, Traits::Error> {
        auto const k = kf_try(Key::create(key));

        usize len = buffer.length();
        auto e = nvs_get_blob(_handle, k.buf, buffer.data(), &len);
        if (ESP_ERR_NVS_INVALID_LENGTH == e) { return error(Traits::Error::BufferTooSmall); }
        if (ESP_OK != e) { return map_nvs_error(e); }
        return ok(BytesView{buffer.data(), len});
    }

    /// @brief Write a variable-length blob under key.
    [[nodiscard]] auto set(StringView key, BytesView data) noexcept -> Traits::ActionResult {
        auto const k = kf_try(Key::create(key));
        return map_nvs_result(nvs_set_blob(_handle, k.buf, data.data(), data.length()));
    }

    /// @brief Query stored blob size without reading it.
    [[nodiscard]] auto size(StringView key) const noexcept -> Result<usize, Traits::Error> {
        auto const k = kf_try(Key::create(key));

        usize len = 0;
        auto e = nvs_get_blob(_handle, k.buf, nullptr, &len);
        if (ESP_OK != e) { return map_nvs_error(e); }
        return ok(len);
    }

    /// @brief Check whether a key exists, regardless of its type.
    [[nodiscard]] auto exists(StringView key) const noexcept -> Result<bool, Traits::Error> {
        auto const k = kf_try(Key::create(key));

        usize len = 0;
        auto e = nvs_get_blob(_handle, k.buf, nullptr, &len);
        if (ESP_ERR_NVS_NOT_FOUND == e) { return ok(false); }
        if (ESP_ERR_NVS_TYPE_MISMATCH == e) { return ok(true); }
        if (ESP_OK == e) { return ok(true); }
        return map_nvs_error(e);
    }

    /// @brief Erase a key, regardless of its type.
    [[nodiscard]] auto erase(StringView key) noexcept -> Traits::ActionResult {
        auto const k = kf_try(Key::create(key));
        return map_nvs_result(nvs_erase_key(_handle, k.buf));
    }

    /// @brief Commit all pending writes since the last commit.
    [[nodiscard]] auto commit() noexcept -> Traits::ActionResult {
        return map_nvs_result(nvs_commit(_handle));
    }

    /// @warning Destroys all namespaces in the partition, not only this one.
    [[nodiscard]] auto erase_partition() noexcept -> Traits::ActionResult {
        deinit_impl();
        _handle = {};
        if (auto e = nvs_flash_erase(); ESP_OK != e) {
            return map_nvs_error(e);
        }
        return init_impl();
    }

private:
    nvs_handle_t _handle{};

#define NVM_NATIVE(bits)                                                                                                            \
    [[nodiscard]] static esp_err_t nvs_get(nvs_handle_t h, char const *k, i##bits *v) noexcept { return nvs_get_i##bits(h, k, v); } \
    [[nodiscard]] static esp_err_t nvs_get(nvs_handle_t h, char const *k, u##bits *v) noexcept { return nvs_get_u##bits(h, k, v); } \
    [[nodiscard]] static esp_err_t nvs_set(nvs_handle_t h, char const *k, i##bits v) noexcept { return nvs_set_i##bits(h, k, v); }  \
    [[nodiscard]] static esp_err_t nvs_set(nvs_handle_t h, char const *k, u##bits v) noexcept { return nvs_set_u##bits(h, k, v); }

    NVM_NATIVE(8)
    NVM_NATIVE(16)
    NVM_NATIVE(32)
    NVM_NATIVE(64)

#undef NVM_NATIVE

    [[nodiscard]] static constexpr auto map_nvs_error(esp_err_t e) noexcept -> internal::ResultErrorWrapper<Traits::Error> {
        using E = Traits::Error;
        switch (e) {
            case ESP_ERR_NVS_NOT_FOUND: return {E::KeyNotFound};
            case ESP_ERR_NVS_TYPE_MISMATCH: return {E::TypeMismatch};
            case ESP_ERR_NVS_KEY_TOO_LONG: return {E::InvalidKey};
            case ESP_ERR_NVS_INVALID_NAME: return {E::InvalidKey};
            case ESP_ERR_NVS_VALUE_TOO_LONG: return {E::ValueTooLong};
            case ESP_ERR_NVS_NOT_ENOUGH_SPACE: return {E::NotEnoughSpace};
            case ESP_ERR_NVS_NO_FREE_PAGES: return {E::NoFreePages};
            case ESP_ERR_NVS_PART_NOT_FOUND: return {E::PartitionNotFound};
            case ESP_ERR_NVS_NEW_VERSION_FOUND: return {E::NewVersionFound};
            default: return {E::Internal};
        }
    }

    [[nodiscard]] static auto map_nvs_result(esp_err_t e) noexcept -> Traits::ActionResult {
        if (ESP_OK == e) { return ok(); }
        return map_nvs_error(e);
    }

    KF_IMPL_DRIVER(NVM);

    auto init_impl() noexcept -> Traits::ActionResult {
        if (auto e = nvs_flash_init(); ESP_OK != e) {
            return map_nvs_error(e);
        }
        if (auto e = nvs_open(this->_config.nvm_namespace, NVS_READWRITE, &_handle); ESP_OK != e) {
            return map_nvs_error(e);
        }
        return ok();
    }

    void deinit_impl() noexcept {
        nvs_close(_handle);
    }
};

#else

struct NVM : internal::NvmBase<NVM> {

    using internal::NvmBase<NVM>::NvmBase;

    /// @brief Read a fixed value of T stored under key.
    template<nvm_fixed T> [[nodiscard]] auto get(StringView key) const noexcept -> Result<T, Traits::Error> {
        return ok(T{});
    }

    /// @brief Write a fixed value of T under key.
    [[nodiscard]] auto set(StringView key, nvm_fixed auto const &value) noexcept -> Traits::ActionResult {
        return ok();
    }

    /// @brief Read a variable-length blob into buffer.
    /// @note Returns a view over actually read bytes - a shorter buffer
    ///       is not an error.
    [[nodiscard]] auto get(StringView key, Bytes buffer) const noexcept -> Result<BytesView, Traits::Error> {
        return ok(BytesView{});
    }

    /// @brief Write a variable-length blob under key.
    [[nodiscard]] auto set(StringView key, BytesView data) noexcept -> Traits::ActionResult {
        return ok();
    }

    /// @brief Query stored blob size without reading it.
    [[nodiscard]] auto size(StringView key) const noexcept -> Result<usize, Traits::Error> {
        return ok<usize>(0);
    }

    /// @brief Check whether a key exists, regardless of its type.
    [[nodiscard]] auto exists(StringView key) const noexcept -> Result<bool, Traits::Error> {
        return ok(false);
    }

    /// @brief Erase a key, regardless of its type.
    [[nodiscard]] auto erase(StringView key) noexcept -> Traits::ActionResult {
        return ok();
    }

    /// @brief Commit all pending writes since the last commit.
    [[nodiscard]] auto commit() noexcept -> Traits::ActionResult {
        return ok();
    }

    /// @warning Destroys all namespaces in the partition, not only this one.
    [[nodiscard]] auto erase_partition() noexcept -> Traits::ActionResult {
        deinit_impl();

        return init_impl();
    }

private:
    KF_IMPL_DRIVER(NVM);

    auto init_impl() noexcept -> Traits::ActionResult {
        return ok();
    }

    void deinit_impl() noexcept {
    }
};

#endif

}// namespace drv

#define CASE_RETURN(__v) \
    case __v: return #__v

[[nodiscard]] constexpr auto repr(drv::NVM::Traits::Error e) noexcept -> StringView {
    switch (e) {
        CASE_RETURN(drv::NVM::Traits::Error::NoSlot);
        CASE_RETURN(drv::NVM::Traits::Error::PartitionNotFound);
        CASE_RETURN(drv::NVM::Traits::Error::NoFreePages);
        CASE_RETURN(drv::NVM::Traits::Error::NewVersionFound);
        CASE_RETURN(drv::NVM::Traits::Error::InvalidKey);
        CASE_RETURN(drv::NVM::Traits::Error::KeyNotFound);
        CASE_RETURN(drv::NVM::Traits::Error::TypeMismatch);
        CASE_RETURN(drv::NVM::Traits::Error::BlobSizeMismatch);
        CASE_RETURN(drv::NVM::Traits::Error::ValueTooLong);
        CASE_RETURN(drv::NVM::Traits::Error::BufferTooSmall);
        CASE_RETURN(drv::NVM::Traits::Error::NotEnoughSpace);
        CASE_RETURN(drv::NVM::Traits::Error::Internal);
    }
    return "drv::NVM::Traits::Error::?";
}

}// namespace kf

#undef CASE_RETURN
