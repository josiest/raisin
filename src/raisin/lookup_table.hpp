#pragma once
#include <type_traits>
#include <string>

namespace raisin {

template<typename iter_t>
concept basic_iterator = std::indirectly_readable<iter_t> and
                         std::weakly_incrementable<iter_t>;

template<basic_iterator iter_t>
using iter_value_t = std::remove_cvref_t<std::iter_reference_t<iter_t>>;

template<typename range_t>
concept searchable = requires(range_t const & range, std::string const & key)
{
    { range.find(key) } -> basic_iterator;
    { range.find(key) } -> std::equality_comparable_with<decltype(range.end())>;
};

template<searchable range_t>
using search_iterator_t = decltype(
        std::declval<range_t>().find(std::declval<std::string>()));

template<typename pair_t>
concept pair = requires(pair_t const & p)
{
    p.first;
    p.second;
};

template<pair pair_t>
using domain_t = std::remove_cvref_t<decltype(std::declval<pair_t>().first)>;

template<pair pair_t>
using codomain_t = std::remove_cvref_t<decltype(std::declval<pair_t>().second)>;

template<typename iter_t>
concept pair_iterator = basic_iterator<iter_t> and
                        pair<iter_value_t<iter_t>>;

template<pair_iterator iter_t>
using iter_domain_t = domain_t<iter_value_t<iter_t>>;

template<pair_iterator iter_t>
using iter_codomain_t = codomain_t<iter_value_t<iter_t>>;

template<typename iter_t, typename key_t, typename value_t>
concept map_iterator = pair_iterator<iter_t> and
                       std::same_as<iter_domain_t<iter_t>, key_t> and
                       std::same_as<iter_codomain_t<iter_t>, value_t>;

template<searchable table_t>
    requires pair_iterator<search_iterator_t<table_t>>
using lookup_key_t = iter_domain_t<search_iterator_t<table_t>>;

template<searchable table_t>
    requires pair_iterator<search_iterator_t<table_t>>
using lookup_value_t = iter_codomain_t<search_iterator_t<table_t>>;

template<typename table_t, typename key_t, typename value_t>
concept lookup_table = searchable<table_t> and
                       map_iterator<search_iterator_t<table_t>, key_t, value_t>;
}
