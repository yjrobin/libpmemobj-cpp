// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2021, 4Paradigm Inc. */

/**
 * @file
 * Atomic specialization for self_relative_ptr.
 */

#ifndef LIBPMEMOBJ_CPP_ATOMIC_SELF_RELATIVE_PTR_HPP
#define LIBPMEMOBJ_CPP_ATOMIC_SELF_RELATIVE_PTR_HPP

#include <libpmemobj++/detail/common.hpp>
#include <libpmemobj++/detail/self_relative_ptr_base_impl.hpp>
#include <libpmemobj++/experimental/self_relative_ptr.hpp>
#include <libpmemobj++/transaction.hpp>

#include <atomic>

namespace std
{
/**
 * Atomic specialization for self_relative_ptr
 *
 * Doesn't automatically add itself to the transaction.
 * The user is responsible for persisting the data.
 */

template <typename T, typename PersistentAware>
struct atomic<pmem::obj::experimental::self_relative_ptr<T, PersistentAware>>;

template <typename T>
struct atomic<pmem::obj::experimental::self_relative_ptr<T, false_type>> {
private:
	using ptr_type = pmem::detail::self_relative_ptr_base_impl<
		std::atomic<std::ptrdiff_t>>;
	using accessor = pmem::detail::self_relative_accessor<
		std::atomic<std::ptrdiff_t>>;

public:
	using this_type = atomic;
	using value_type =
		pmem::obj::experimental::self_relative_ptr<T, false_type>;
	using difference_type = typename value_type::difference_type;

	/*
	 * Constructors
	 */

	constexpr atomic() noexcept = default;
	atomic(value_type value) : ptr()
	{
		store(value);
	}
	atomic(const atomic &) = delete;

	void
	store(value_type desired,
	      std::memory_order order = std::memory_order_seq_cst) noexcept
	{
		auto offset = accessor::pointer_to_offset(ptr, desired.get());
		LIBPMEMOBJ_CPP_ANNOTATE_HAPPENS_BEFORE(order, &ptr);
		accessor::get_offset(ptr).store(offset, order);
	}

	value_type
	load(std::memory_order order = std::memory_order_seq_cst) const noexcept
	{
		auto offset = accessor::get_offset(ptr).load(order);
		LIBPMEMOBJ_CPP_ANNOTATE_HAPPENS_AFTER(order, &ptr);
		auto pointer = accessor::offset_to_pointer<T>(offset, ptr);
		return value_type{pointer};
	}

	value_type
	exchange(value_type desired,
		 std::memory_order order = std::memory_order_seq_cst) noexcept
	{
		auto new_offset =
			accessor::pointer_to_offset(ptr, desired.get());
		auto old_offset =
			accessor::get_offset(ptr).exchange(new_offset, order);
		return value_type{
			accessor::offset_to_pointer<T>(old_offset, ptr)};
	}

	bool
	compare_exchange_weak(value_type &expected, value_type desired,
			      std::memory_order success,
			      std::memory_order failure) noexcept
	{
		auto expected_offset =
			accessor::pointer_to_offset(ptr, expected.get());
		auto desired_offset =
			accessor::pointer_to_offset(ptr, desired.get());
		bool result = accessor::get_offset(ptr).compare_exchange_weak(
			expected_offset, desired_offset, success, failure);
		if (!result) {
			try {
				expected = accessor::offset_to_pointer<T>(
					expected_offset, ptr);
			} catch (...) {
				std::terminate();
			}
		}
		return result;
	}

	bool
	compare_exchange_weak(
		value_type &expected, value_type desired,
		std::memory_order order = std::memory_order_seq_cst) noexcept
	{
		auto expected_offset =
			accessor::pointer_to_offset(ptr, expected.get());
		auto desired_offset =
			accessor::pointer_to_offset(ptr, desired.get());
		bool result = accessor::get_offset(ptr).compare_exchange_weak(
			expected_offset, desired_offset, order);
		if (!result) {
			try {
				expected = accessor::offset_to_pointer<T>(
					expected_offset, ptr);
			} catch (...) {
				std::terminate();
			}
		}
		return result;
	}

	bool
	compare_exchange_strong(value_type &expected, value_type desired,
				std::memory_order success,
				std::memory_order failure) noexcept
	{
		auto expected_offset =
			accessor::pointer_to_offset(ptr, expected.get());
		auto desired_offset =
			accessor::pointer_to_offset(ptr, desired.get());
		bool result = accessor::get_offset(ptr).compare_exchange_strong(
			expected_offset, desired_offset, success, failure);
		if (!result) {
			try {
				expected = accessor::offset_to_pointer<T>(
					expected_offset, ptr);
			} catch (...) {
				std::terminate();
			}
		}
		return result;
	}

	bool
	compare_exchange_strong(
		value_type &expected, value_type desired,
		std::memory_order order = std::memory_order_seq_cst) noexcept
	{
		auto expected_offset =
			accessor::pointer_to_offset(ptr, expected.get());
		auto desired_offset =
			accessor::pointer_to_offset(ptr, desired.get());
		bool result = accessor::get_offset(ptr).compare_exchange_strong(
			expected_offset, desired_offset, order);
		if (!result) {
			try {
				expected = accessor::offset_to_pointer<T>(
					expected_offset, ptr);
			} catch (...) {
				std::terminate();
			}
		}
		return result;
	}

	value_type
	fetch_add(difference_type val,
		  std::memory_order order = std::memory_order_seq_cst) noexcept
	{
		auto offset = accessor::get_offset(ptr).fetch_add(
			val * static_cast<difference_type>(sizeof(T)), order);
		return value_type{accessor::offset_to_pointer<T>(offset, ptr)};
	}

	value_type
	fetch_sub(difference_type val,
		  std::memory_order order = std::memory_order_seq_cst) noexcept
	{
		auto offset = accessor::get_offset(ptr).fetch_sub(
			val * static_cast<difference_type>(sizeof(T)), order);
		return value_type{accessor::offset_to_pointer<T>(offset, ptr)};
	}

	bool
	is_lock_free() const noexcept
	{
		return accessor::get_offset(ptr).is_lock_free();
	}

	/*
	 * Operators
	 */

	operator value_type() const noexcept
	{
		return load();
	}

	atomic &operator=(const atomic &) = delete;
	atomic &operator=(const atomic &) volatile = delete;

	value_type
	operator=(value_type desired) noexcept
	{
		store(desired);
		return desired;
	}

	value_type
	operator++() noexcept
	{
		try {
			return this->fetch_add(1) + 1;
		} catch (...) {
			/* This should never happen during normal program
			 * execution */
			std::terminate();
		}
	}

	value_type
	operator++(int) noexcept
	{
		return this->fetch_add(1);
	}

	value_type
	operator--() noexcept
	{
		try {
			return this->fetch_sub(1) - 1;
		} catch (...) {
			/* This should never happen during normal program
			 * execution */
			std::terminate();
		}
	}

	value_type
	operator--(int) noexcept
	{
		return this->fetch_sub(1);
	}

	value_type
	operator+=(difference_type diff) noexcept
	{
		try {
			return this->fetch_add(diff) + diff;
		} catch (...) {
			/* This should never happen during normal program
			 * execution */
			std::terminate();
		}
	}

	value_type
	operator-=(difference_type diff) noexcept
	{
		try {
			return this->fetch_sub(diff) - diff;
		} catch (...) {
			/* This should never happen during normal program
			 * execution */
			std::terminate();
		}
	}

	pmem::obj::pool_base
	get_pool_base() const
	{
		PMEMobjpool *pop = pmemobj_pool_by_ptr(this);
		return pmem::obj::pool_base(pop);
	}

private:
	ptr_type ptr;
};

template <typename T>
struct atomic<pmem::obj::experimental::self_relative_ptr<T, true_type>> {
private:
	using ptr_type = pmem::detail::self_relative_ptr_base_impl<
		std::atomic<std::ptrdiff_t>>;
	using accessor = pmem::detail::self_relative_accessor<
		std::atomic<std::ptrdiff_t>>;

public:
	using this_type = atomic;
	using value_type =
		pmem::obj::experimental::self_relative_ptr<T, true_type>;
	using difference_type = typename value_type::difference_type;

	/*
	 * Constructors
	 */

	constexpr atomic() noexcept = default;
	atomic(value_type value) : ptr()
	{
		store(value);
	}
	atomic(const atomic &) = delete;

	void
	store(value_type desired,
	      std::memory_order order = std::memory_order_seq_cst) noexcept
	{
		auto offset = accessor::pointer_to_offset(ptr, desired.get());
		LIBPMEMOBJ_CPP_ANNOTATE_HAPPENS_BEFORE(order, &ptr);
		intptr_t dirty_mask = desired.is_dirty();
		--dirty_mask;
		offset &= (dirty_mask | ptr_type::dirty_flag);
		accessor::get_offset(ptr).store(offset, order);
	}

	value_type
	load(std::memory_order order = std::memory_order_seq_cst) const noexcept
	{
		auto offset = accessor::get_offset(ptr).load(order);
		LIBPMEMOBJ_CPP_ANNOTATE_HAPPENS_AFTER(order, &ptr);
		if (ptr_type::is_dirty(offset)) {
			auto &target = accessor::get_offset(ptr);
			get_pool_base().persist(&target, sizeof(target));
		}
		auto pointer = accessor::offset_to_pointer<T>(offset, ptr);
		return value_type{pointer, ptr_type::is_dirty(offset)};
	}

	value_type
	persist_load(
		std::memory_order order = std::memory_order_seq_cst) noexcept
	{
		auto offset = accessor::get_offset(ptr).load(order);
		LIBPMEMOBJ_CPP_ANNOTATE_HAPPENS_AFTER(order, &ptr);
		intptr_t dirty_mask;
		if ((dirty_mask = (ptr_type::is_dirty(offset) == true)) > 0) {
			auto &target = accessor::get_offset(ptr);
			get_pool_base().persist(&target, sizeof(target));
		}
		auto desired = offset;
		while (true) {
			--dirty_mask;
			desired = offset | ~(ptr_type::dirty_flag | dirty_mask);
			if (accessor::get_offset(ptr).compare_exchange_weak(
				    offset, desired) ||
			    !ptr_type::is_dirty(offset)) {
				break;
			}
			auto &target = accessor::get_offset(ptr);
			get_pool_base().persist(&target, sizeof(target));
			dirty_mask = ptr_type::is_dirty(offset) == true;
		}
		auto pointer = accessor::offset_to_pointer<T>(offset, ptr);
		return value_type{pointer, ptr_type::is_dirty(offset)};
	}
	value_type
	exchange(value_type desired,
		 std::memory_order order = std::memory_order_seq_cst) noexcept
	{
		auto new_offset =
			accessor::pointer_to_offset(ptr, desired.get());
		intptr_t dirty_mask = desired.is_dirty();
		--dirty_mask;
		new_offset &= (dirty_mask | ptr_type::dirty_flag);
		auto old_offset =
			accessor::get_offset(ptr).exchange(new_offset, order);
		return value_type{
			accessor::offset_to_pointer<T>(old_offset, ptr),
			ptr_type::is_dirty(old_offset)};
	}

	bool
	compare_exchange_weak(value_type &expected, value_type desired,
			      std::memory_order success,
			      std::memory_order failure) noexcept
	{
		auto expected_offset =
			accessor::pointer_to_offset(ptr, expected.get());
		auto desired_offset =
			accessor::pointer_to_offset(ptr, desired.get());
		intptr_t dirty_mask = expected.is_dirty();
		--dirty_mask;
		expected_offset &= (dirty_mask | ptr_type::dirty_flag);
		dirty_mask = desired.is_dirty();
		--dirty_mask;
		desired_offset &= (dirty_mask | ptr_type::dirty_flag);
		bool result = accessor::get_offset(ptr).compare_exchange_weak(
			expected_offset, desired_offset, success, failure);
		if (!result) {
			try {
				expected = accessor::offset_to_pointer<T>(
					expected_offset, ptr);
				expected.set_dirty_flag(
					ptr_type::is_dirty(expected_offset));
			} catch (...) {
				std::terminate();
			}
		}
		return result;
	}

	bool
	compare_exchange_weak(
		value_type &expected, value_type desired,
		std::memory_order order = std::memory_order_seq_cst) noexcept
	{
		auto expected_offset =
			accessor::pointer_to_offset(ptr, expected.get());
		auto desired_offset =
			accessor::pointer_to_offset(ptr, desired.get());
		intptr_t dirty_mask = expected.is_dirty();
		--dirty_mask;
		expected_offset &= (dirty_mask | ptr_type::dirty_flag);
		dirty_mask = desired.is_dirty();
		--dirty_mask;
		desired_offset &= (dirty_mask | ptr_type::dirty_flag);
		bool result = accessor::get_offset(ptr).compare_exchange_weak(
			expected_offset, desired_offset, order);
		if (!result) {
			try {
				expected = accessor::offset_to_pointer<T>(
					expected_offset, ptr);
				expected.set_dirty_flag(
					ptr_type::is_dirty(expected_offset));
			} catch (...) {
				std::terminate();
			}
		}
		return result;
	}

	bool
	compare_exchange_strong(value_type &expected, value_type desired,
				std::memory_order success,
				std::memory_order failure) noexcept
	{
		auto expected_offset =
			accessor::pointer_to_offset(ptr, expected.get());
		auto desired_offset =
			accessor::pointer_to_offset(ptr, desired.get());
		intptr_t dirty_mask = expected.is_dirty();
		--dirty_mask;
		expected_offset &= (dirty_mask | ptr_type::dirty_flag);
		dirty_mask = desired.is_dirty();
		--dirty_mask;
		desired_offset &= (dirty_mask | ptr_type::dirty_flag);
		bool result = accessor::get_offset(ptr).compare_exchange_strong(
			expected_offset, desired_offset, success, failure);
		if (!result) {
			try {
				expected = accessor::offset_to_pointer<T>(
					expected_offset, ptr);
				expected.set_dirty_flag(
					ptr_type::is_dirty(expected_offset));
			} catch (...) {
				std::terminate();
			}
		}
		return result;
	}

	bool
	compare_exchange_strong(
		value_type &expected, value_type desired,
		std::memory_order order = std::memory_order_seq_cst) noexcept
	{
		auto expected_offset =
			accessor::pointer_to_offset(ptr, expected.get());
		auto desired_offset =
			accessor::pointer_to_offset(ptr, desired.get());
		intptr_t dirty_mask = expected.is_dirty();
		--dirty_mask;
		expected_offset &= (dirty_mask | ptr_type::dirty_flag);
		dirty_mask = desired.is_dirty();
		--dirty_mask;
		desired_offset &= (dirty_mask | ptr_type::dirty_flag);
		bool result = accessor::get_offset(ptr).compare_exchange_strong(
			expected_offset, desired_offset, order);
		if (!result) {
			try {
				expected = accessor::offset_to_pointer<T>(
					expected_offset, ptr);
				expected.set_dirty_flag(
					ptr_type::is_dirty(expected_offset));
			} catch (...) {
				std::terminate();
			}
		}
		return result;
	}

	value_type
	fetch_add(difference_type val,
		  std::memory_order order = std::memory_order_seq_cst) noexcept
	{
		auto offset = accessor::get_offset(ptr).fetch_add(
			val * static_cast<difference_type>(sizeof(T)), order);
		return value_type{accessor::offset_to_pointer<T>(offset, ptr),
				  true};
	}

	value_type
	fetch_sub(difference_type val,
		  std::memory_order order = std::memory_order_seq_cst) noexcept
	{
		auto offset = accessor::get_offset(ptr).fetch_sub(
			val * static_cast<difference_type>(sizeof(T)), order);
		return value_type{accessor::offset_to_pointer<T>(offset, ptr),
				  true};
	}

	bool
	is_lock_free() const noexcept
	{
		return accessor::get_offset(ptr).is_lock_free();
	}

	/*
	 * Operators
	 */

	operator value_type() const noexcept
	{
		return load();
	}

	atomic &operator=(const atomic &) = delete;
	atomic &operator=(const atomic &) volatile = delete;

	value_type
	operator=(value_type desired) noexcept
	{
		store(desired);
		return desired;
	}

	value_type
	operator&=(difference_type flag) noexcept
	{
		auto old_offset = accessor::get_offset(ptr).load();
		auto new_offset = (old_offset & flag);
		accessor::get_offset(ptr).exchange(new_offset);
		return load();
	}

	value_type
	operator++() noexcept
	{
		try {
			return this->fetch_add(1) + 1;
		} catch (...) {
			/* This should never happen during normal program
			 * execution */
			std::terminate();
		}
	}

	value_type
	operator++(int) noexcept
	{
		return this->fetch_add(1);
	}

	value_type
	operator--() noexcept
	{
		try {
			return this->fetch_sub(1) - 1;
		} catch (...) {
			/* This should never happen during normal program
			 * execution */
			std::terminate();
		}
	}

	value_type
	operator--(int) noexcept
	{
		return this->fetch_sub(1);
	}

	value_type
	operator+=(difference_type diff) noexcept
	{
		try {
			return this->fetch_add(diff) + diff;
		} catch (...) {
			/* This should never happen during normal program
			 * execution */
			std::terminate();
		}
	}

	value_type
	operator-=(difference_type diff) noexcept
	{
		try {
			return this->fetch_sub(diff) - diff;
		} catch (...) {
			/* This should never happen during normal program
			 * execution */
			std::terminate();
		}
	}

	pmem::obj::pool_base
	get_pool_base() const
	{
		PMEMobjpool *pop = pmemobj_pool_by_ptr(this);
		return pmem::obj::pool_base(pop);
	}

private:
	ptr_type ptr;
};

} /* namespace std */

namespace pmem
{

namespace detail
{

/**
 * pmem::detail::can_do_snapshot atomic specialization for self_relative_ptr.
 * Not thread safe.
 *
 * Use in a single-threaded environment only.
 */
template <typename T, typename PersistentAware>
struct can_do_snapshot<
	std::atomic<obj::experimental::self_relative_ptr<T, PersistentAware>>> {
	using snapshot_type =
		obj::experimental::self_relative_ptr<T, PersistentAware>;
	static constexpr bool value = sizeof(std::atomic<snapshot_type>) ==
		sizeof(typename snapshot_type::offset_type);
	static_assert(value,
		      "std::atomic<self_relative_ptr> should be the same size");
};

} /* namespace detail */

} /* namespace pmem */

#endif
