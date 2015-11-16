#include "orm/alias.h"
#define CATCH_CONFIG_MAIN
#include "catch.hpp"

using namespace Yb;

TEST_CASE( "build up alias with limited length", "[mk_alias]" ) {
    REQUIRE( mk_alias("t_client", "name", 14, 1) == "t_client_name" );
    REQUIRE( mk_alias("t_client", "name", 13, 1) == "t_client_name" );
    REQUIRE( mk_alias("t_client", "name", 12, 1) == "t_client_n_1" );
    REQUIRE( mk_alias("t_client", "name", 12, 12) == "t_client__12" );
    REQUIRE( mk_alias("t_client", "name", 12, 123) == "t_client_123" );
    REQUIRE( mk_alias("t_client", "xname", 13, 123) == "t_client__123" );
}

TEST_CASE( "camelCase detection", "[is_camel]" ) {
    REQUIRE( !is_camel("a") );
    REQUIRE( !is_camel("A") );
    REQUIRE( !is_camel("ab") );
    REQUIRE( !is_camel("AB") );
    REQUIRE( !is_camel("a_b") );
    REQUIRE( !is_camel("A_B") );
    REQUIRE( is_camel("aB") );
    REQUIRE( is_camel("Ab") );
    REQUIRE( is_camel("a_B") );
    REQUIRE( is_camel("A_b") );
}

TEST_CASE( "split an ID into words", "[split_words]" ) {
    string_vector words, words0;
    words0.push_back("ab");
    words0.push_back("ra");
    words0.push_back("cada");
    words0.push_back("bra");
    split_words("abRaCadaBra", words);
    REQUIRE( words == words0 );
    words.clear();
    split_words("AbRaCadaBra", words);
    REQUIRE( words == words0 );
    words.clear();
    split_words("ab_RaCada_bra", words);
    REQUIRE( words == words0 );
    words.clear();
    split_words("_ab_ra___cada_bra_", words);
    REQUIRE( words == words0 );
}

TEST_CASE( "check if it is a vowel (lowercase)", "[is_vowel]" ) {
    REQUIRE( is_vowel('a') );
    REQUIRE( !is_vowel('y') ); 
    REQUIRE( !is_vowel('p') ); 
}

TEST_CASE( "check if we can skip a letter when shortening", "[skip_letter]" ) {
    REQUIRE( skip_letter('k', 'e', 'k') == true );
    REQUIRE( skip_letter('p', 'r', 'i') == false );
    REQUIRE( skip_letter('\0', 'y', 'a') == false );
    REQUIRE( skip_letter('d', 'y', 'n') == true );
    REQUIRE( skip_letter('t', 'y', 'a') == false );
    REQUIRE( skip_letter('d', 'h', 'a') == true );
    REQUIRE( skip_letter('d', 'h', 'd') == true );
    REQUIRE( skip_letter('\0', 'h', 'a') == false );
    REQUIRE( skip_letter('o', 'h', 'u') == false );
}

TEST_CASE( "shorten given word", "[shorten]" ) {
    REQUIRE( shorten("id") == "id" );
    REQUIRE( shorten("paysys") == "pyss" );
    REQUIRE( shorten("payment") == "pymnt" );
    REQUIRE( shorten("method") == "mtd" );
    REQUIRE( shorten("hidden") == "hddn" );
    REQUIRE( shorten("year") == "yr" );
    REQUIRE( shorten("simply") == "smpl" );
    REQUIRE( shorten("order") == "ordr" );
}

TEST_CASE( "find conflicting aliases", "[get_conflicts]" ) {
    string_vector ctables0, ctables;
    ctables0.push_back("account");
    ctables0.push_back("act");
    ctables0.push_back("consume");
    ctables0.push_back("contract");
    string_map aliases;
    aliases["invoice"] = "i";
    aliases["act"] = "a";
    aliases["account"] = "a";
    aliases["consume"] = "c";
    aliases["contract"] = "c";
    aliases["person"] = "p";
    string_set out;
    get_conflicts(aliases, out);
    std::copy(out.begin(), out.end(), std::back_inserter(ctables));
    REQUIRE( ctables == ctables0 );
}

TEST_CASE( "build up table aliases from abbreviations", "[mk_table_aliases]" ) {
    str2strvec_map tbl_words;
    string_vector v;
    v.push_back("pymnt");
    v.push_back("mtd");
    tbl_words["t_payment_method"] = v;
    v.clear();
    v.push_back("cntrct");
    tbl_words["t_contract"] = v;
    str2intvec_map word_pos;
    int_vector iv;
    iv.push_back(2);
    iv.push_back(1);
    word_pos["t_payment_method"] = iv;
    iv.clear();
    iv.push_back(2);
    word_pos["t_contract"] = iv;
    string_map out;
    mk_table_aliases(tbl_words, word_pos, out);
    REQUIRE( out.size() == 2 );
    REQUIRE( out["t_contract"] == "cn" );
    REQUIRE( out["t_payment_method"] == "pym" );
}

TEST_CASE( "get fallback aliases in case of conflics", "[fallback_table_aliases]" ) {
    string_set confl;
    confl.insert("t_longtable1");
    confl.insert("t_longtable2");
    str2strvec_map tbl_words;
    string_vector v;
    v.push_back("pymnt");
    v.push_back("mtd");
    tbl_words["t_payment_method"] = v;
    v.clear();
    v.push_back("lngtbl2");
    tbl_words["t_longtable2"] = v;
    v.clear();
    v.push_back("cntrct");
    tbl_words["t_contract"] = v;
    v.clear();
    v.push_back("lngtbl1");
    tbl_words["t_longtable1"] = v;
    str2intvec_map word_pos;
    int_vector iv;
    iv.push_back(2);
    iv.push_back(1);
    word_pos["t_payment_method"] = iv;
    iv.clear();
    iv.push_back(2);
    word_pos["t_contract"] = iv;
    iv.clear();
    iv.push_back(4);
    word_pos["t_longtable1"] = iv;
    word_pos["t_longtable2"] = iv;
    string_map out;
    fallback_table_aliases(confl, tbl_words, word_pos, out);
    REQUIRE( out.size() == 4 );
    REQUIRE( out["t_contract"] == "cn" );
    REQUIRE( out["t_payment_method"] == "pym" );
    REQUIRE( out["t_longtable1"] == "lngt1" );
    REQUIRE( out["t_longtable2"] == "lngt2" );
}

TEST_CASE ( "find aliases for the set of tables", "[table_aliases]" ) {
    {
        string_set t;
        t.insert("client");
        t.insert("order");
        string_map a;
        table_aliases(t, a);
        REQUIRE( a.size() == 2 );
        REQUIRE( a["client"] == "c" );
        REQUIRE( a["order"] == "o" );
    }
    {
        string_set t;
        t.insert("t_paysys");
        t.insert("t_payment_method");
        t.insert("t_order");
        string_map a;
        table_aliases(t, a);
        REQUIRE( a.size() == 3 );
        REQUIRE( a["t_paysys"] == "p" );
        REQUIRE( a["t_order"] == "o" );
        REQUIRE( a["t_payment_method"] == "pm" );
    }
    {
        string_set t;
        t.insert("client");
        t.insert("contract");
        t.insert("order");
        string_map a;
        table_aliases(t, a);
        REQUIRE( a.size() == 3 );
        REQUIRE( a["client"] == "cl" );
        REQUIRE( a["contract"] == "cn" );
        REQUIRE( a["order"] == "o" );
    }
    {
        string_set t;
        t.insert("pomidoro");
        t.insert("t_payment_method");
        t.insert("PayMethod");
        string_map a;
        table_aliases(t, a);
        REQUIRE( a.size() == 3 );
        REQUIRE( a["pomidoro"] == "p" );
        REQUIRE( a["t_payment_method"] == "pymm" );
        REQUIRE( a["PayMethod"] == "pymt" );
    }
    {
        string_set t;
        t.insert("abc");
        t.insert("longtable1");
        t.insert("longtable4");
        t.insert("longtable3");
        string_map a;
        table_aliases(t, a);
        REQUIRE( a.size() == 4 );
        REQUIRE( a["abc"] == "a" );
        REQUIRE( a["longtable1"] == "lngtb1" );
        REQUIRE( a["longtable3"] == "lngtb2" );
        REQUIRE( a["longtable4"] == "lngtb3" );
    }
    {
        string_set t;
        t.insert("abc");
        t.insert("longt1");
        t.insert("longt4");
        t.insert("longt3");
        string_map a;
        table_aliases(t, a);
        REQUIRE( a.size() == 4 );
        REQUIRE( a["abc"] == "a" );
        REQUIRE( a["longt1"] == "lngt1" );
        REQUIRE( a["longt3"] == "lngt3" );
        REQUIRE( a["longt4"] == "lngt4" );
    }
}

TEST_CASE ( "build up aliases for columns", "[col_aliases]" ) {
    {
        strpair_vector p;
        p.push_back(std::make_pair("t_client", "name"));
        p.push_back(std::make_pair("t_payment_method", "paysys_id"));
        string_vector aliases, aliases0;
        aliases0.push_back("c_name");
        aliases0.push_back("pm_paysys_id");
        col_aliases(p, 12, aliases);
        REQUIRE( aliases0 == aliases );
    }
    {
        strpair_vector p;
        p.push_back(std::make_pair("t_paysys", "name"));
        p.push_back(std::make_pair("t_passport", "uid"));
        p.push_back(std::make_pair("t_payment_method", "paysys_id"));
        string_vector aliases, aliases0;
        aliases0.push_back("py_name");
        aliases0.push_back("ps_uid");
        aliases0.push_back("pm_paysys_3");
        col_aliases(p, 11, aliases);
        REQUIRE( aliases0 == aliases );
    }
}

