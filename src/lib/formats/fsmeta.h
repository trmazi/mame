// license:BSD-3-Clause
// copyright-holders:Olivier Galibert

// Filesystem metadata management

#ifndef MAME_FORMATS_FSMETA_H
#define MAME_FORMATS_FSMETA_H

#pragma once

#include "timeconv.h"

#include <cassert>
#include <functional>
#include <string>
#include <unordered_map>
#include <variant>

namespace fs {

enum class meta_name {
	basic,
	creation_date,
	length,
	loading_address,
	locked,
	modification_date,
	name,
	os_minimum_version,
	os_version,
	rsrc_length,
	sequential,
	size_in_blocks,
	file_type,
	ascii_flag,
	owner_id,
	attributes
};

enum class meta_type {
	date,
	flag,
	number,
	string,
};

class meta_value {
public:
	static std::string to_string(meta_type type, const meta_value &m);
	static meta_value from_string(meta_type type, std::string value);

	meta_value() { value = false; }
	meta_value(std::string str) { value = str; }
	meta_value(bool b) { value = b; }
	meta_value(int32_t num) { value = uint64_t(num); }
	meta_value(uint32_t num) { value = uint64_t(num); }
	meta_value(int64_t num) { value = uint64_t(num); }
	meta_value(uint64_t num) { value = num; }
	meta_value(util::arbitrary_datetime dt) { value = dt; }

	util::arbitrary_datetime as_date() const { return *std::get_if<util::arbitrary_datetime>(&value); }
	bool as_flag() const { return *std::get_if<bool>(&value); }
	uint64_t as_number() const { return *std::get_if<uint64_t>(&value); }
	std::string as_string() const { return *std::get_if<std::string>(&value); }

private:
	std::variant<std::string, uint64_t, bool, util::arbitrary_datetime> value;
};

class meta_data {
public:
	std::unordered_map<meta_name, meta_value> meta;

	static const char *entry_name(meta_name name);

	bool has(meta_name name) const { return meta.find(name) != meta.end(); }
	bool empty() const { return meta.empty(); }

	void set(meta_name name, const meta_value &val) { meta[name] = val; }
	void set(meta_name name, std::string str) { set(name, meta_value(str)); }
	void set(meta_name name, bool b) { set(name, meta_value(b)); }
	void set(meta_name name, int32_t num) { set(name, meta_value(num)); }
	void set(meta_name name, uint32_t num) { set(name, meta_value(num)); }
	void set(meta_name name, int64_t num) { set(name, meta_value(num)); }
	void set(meta_name name, uint64_t num) { set(name, meta_value(num)); }
	void set(meta_name name, util::arbitrary_datetime dt) { set(name, meta_value(dt)); }
	void set_now(meta_name name) { set(name, meta_value(util::arbitrary_datetime::now())); }

	meta_value get(meta_name name) const { auto i = meta.find(name);  assert(i != meta.end()); return i->second; }
	util::arbitrary_datetime get_date(meta_name name, util::arbitrary_datetime def = util::arbitrary_datetime::now()) const { auto i = meta.find(name);  if(i == meta.end()) return def; else return i->second.as_date(); }
	bool get_flag(meta_name name, bool def = false) const { auto i = meta.find(name);  if(i == meta.end()) return def; else return i->second.as_flag(); }
	uint64_t get_number(meta_name name, uint64_t def = 0) const { auto i = meta.find(name);  if(i == meta.end()) return def; else return i->second.as_number(); }
	std::string get_string(meta_name name, std::string def = "") const { auto i = meta.find(name);  if(i == meta.end()) return def; else return i->second.as_string(); }
};

struct meta_description {
	meta_name m_name;
	meta_type m_type;
	meta_value m_default;
	bool m_ro;
	std::function<void (const meta_value &)> m_validator;
	const char *m_tooltip;

	meta_description(meta_name name, meta_type type, int def, bool ro, std::function<void (meta_value)> validator, const char *tooltip) :
		m_name(name), m_type(type), m_default(uint64_t(def)), m_ro(ro), m_validator(validator), m_tooltip(tooltip)
	{}

	template<typename T> meta_description(meta_name name, meta_type type, T def, bool ro, std::function<void (meta_value)> validator, const char *tooltip) :
		m_name(name), m_type(type), m_default(def), m_ro(ro), m_validator(validator), m_tooltip(tooltip)
	{}
};

} // namespace fs

#endif // MAME_FORMATS_FSMETA_H
