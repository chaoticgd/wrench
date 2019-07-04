/*
	wrench - A set of modding tools for the Ratchet & Clank PS2 games.
	Copyright (C) 2019 chaoticgd

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef TREE_H
#define TREE_H
#include <memory>
#include <vector>
#include <algorithm>
#include <stdexcept>

template <typename T>
class tree_node {
protected:
	tree_node()
			: _parent(nullptr) {}
	virtual ~tree_node() {}

public:
	T* child_at(std::size_t index) const {
		return _children[index].get();
	}

	std::vector<T*> children() const {
		std::vector<T*> result(_children.size());
		std::transform(
			_children.begin(), _children.end(), result.begin(),
			[](const std::unique_ptr<T>& child) { return child.get(); });
		return result;
	}

	std::size_t num_children() const {
		return _children.size();
	}

	template <typename T_child_type>
	T_child_type* first_child_of_type() const {
		for(const std::unique_ptr<T>& base_child : _children) {
			T_child_type* child = dynamic_cast<T_child_type*>(base_child.get());
			if(child != nullptr) {
				return child;
			}
		}
		throw std::out_of_range("No direct child of the correct type exists.");
	}

	template <typename T_child_type>
	std::vector<T_child_type*> children_of_type() const {
		std::vector<T_child_type*> result;
		for(const std::unique_ptr<T>& base_child : _children) {
			T_child_type* child = dynamic_cast<T_child_type*>(base_child.get());
			if(child != nullptr) {
				result.push_back(child);
			}
		}
		return result;
	}

	template <typename T_child, typename... T_constructor_args>
	T* emplace_child(T_constructor_args... args) {
		auto child = std::make_unique<T_child>(args...);
		static_cast<tree_node<T>*>(child.get())->_parent = static_cast<T*>(this);
		return append_child(std::move(child));
	}

	T* append_child(std::unique_ptr<T> child) {
		return insert_child(std::move(child), _children.size());
	}

	T* insert_child(std::unique_ptr<T> child, size_t position) {
		static_cast<tree_node<T>*>(child.get())->_parent = static_cast<T*>(this);
		T* raw = child.get();
		_children.insert(_children.begin() + position, std::move(child));
		return raw;
	}

	std::unique_ptr<T> remove_child(T* child) {
		auto iter = std::find_if(_children.begin(), _children.end(),
			[=](std::unique_ptr<T>& current) { return current.get() == child; });
		std::unique_ptr<T> orphaned_child = std::move(*iter);
		orphaned_child->_parent = nullptr;
		_children.erase(iter);
		return orphaned_child;
	}

	T* parent() {
		return _parent;
	}

	bool has_parent() {
		return _parent != nullptr;
	}

	T* root() {
		T* current = static_cast<T*>(this);
		while(current->has_parent()) {
			current = static_cast<tree_node<T>*>(current)->_parent;
		}
		return current;
	}

private:
	std::vector<std::unique_ptr<T>> _children;
	T* _parent;
};

#endif
