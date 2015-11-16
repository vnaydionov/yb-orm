
def mk_alias(tbl, col, max_len, cnt):
    '''
    >>> mk_alias('t_client', 'name', 14, 1)
    't_client_name'
    >>> mk_alias('t_client', 'name', 13, 1)
    't_client_name'
    >>> mk_alias('t_client', 'name', 12, 1)
    't_client_n_1'
    >>> mk_alias('t_client', 'name', 12, 12)
    't_client__12'
    >>> mk_alias('t_client', 'name', 12, 123)
    't_client_123'
    >>> mk_alias('t_client', 'xname', 13, 123)
    't_client__123'
    '''
    return tbl + '_' + col if len(tbl) + 1 + len(col) <= max_len else (tbl + '_' + col)[:max_len - len(str(cnt)) - 1] + '_' + str(cnt)

def is_camel(s):
    '''
    >>> is_camel('a')
    False
    >>> is_camel('A')
    False
    >>> is_camel('ab')
    False
    >>> is_camel('AB')
    False
    >>> is_camel('a_b')
    False
    >>> is_camel('A_B')
    False
    >>> is_camel('aB')
    True
    >>> is_camel('Ab')
    True
    >>> is_camel('a_B')
    True
    >>> is_camel('A_b')
    True
    '''
    return s.upper() != s and s.lower() != s

def split_words(s):
    '''
    >>> split_words('abRaCadaBra')
    ['ab', 'ra', 'cada', 'bra']
    >>> split_words('AbRaCadaBra')
    ['ab', 'ra', 'cada', 'bra']
    >>> split_words('ab_RaCada_bra')
    ['ab', 'ra', 'cada', 'bra']
    >>> split_words('_ab_ra___cada_bra_')
    ['ab', 'ra', 'cada', 'bra']
    '''
    return [w for w in
            ''.join(c if not (c >= 'A' and c <= 'Z') else '_' + c for c in s).strip('_').lower().split('_')
            if w]

def main_words(tbl):
    '''
    >>> main_words('abRaCadaBra')
    ['ab', 'ra', 'cada', 'bra']
    >>> main_words('a_bRaCadaBra')
    ['ra', 'cada', 'bra']
    '''
    return [w for w in split_words(tbl) if len(w) > 1]

def is_vowel(c):
    '''
    >>> is_vowel('a')
    True
    >>> is_vowel('y')
    False
    '''
    return c in 'aouei'

def skip_letter(p, c, n):
    '''
    >>> skip_letter('k', 'e', 'k')
    True
    >>> skip_letter('p', 'r', 'i')
    False
    >>> skip_letter('', 'y', 'a')
    False
    >>> skip_letter('d', 'y', 'n')
    True
    >>> skip_letter('t', 'y', 'a')
    False
    >>> skip_letter('d', 'h', 'a')
    True
    >>> skip_letter('d', 'h', 'd')
    True
    >>> skip_letter('', 'h', 'a')
    False
    >>> skip_letter('o', 'h', 'u')
    False
    '''
    return bool((is_vowel(c) and p)
                or (c == 'y' and n and not is_vowel(n) and p and not is_vowel(p))
                or (c == 'y' and not n and p and not is_vowel(p))
                or (c == 'h' and n and not is_vowel(n))
                or (c == 'h' and p and not is_vowel(p)))

def get_consonants(s):
    '''
    >>> get_consonants('paysys')
    'pyss'
    >>> get_consonants('payment')
    'pymnt'
    >>> get_consonants('method')
    'mtd'
    >>> get_consonants('hidden')
    'hddn'
    >>> get_consonants('year')
    'yr'
    >>> get_consonants('simply')
    'smpl'
    >>> get_consonants('order')
    'ordr'
    '''
    return ''.join(s[i] for i in range(len(s)) if not skip_letter(''.join(s[i-1:i]), s[i], ''.join(s[i+1:i+2])))

def shorten(s):
    s = s.lower()
    return s if s == 'id' else get_consonants(s)

def alias(tbl, col, max_len, cnt):
    '''
    >>> alias('t_client', 'name', 20, 1)
    'clnt_nm'
    >>> alias('t_payment_method', 'paysys_id', 20, 1)
    'pymnt_mtd_pyss_id'
    '''
    return mk_alias(
            '_'.join(shorten(w) for w in main_words(tbl)),
            '_'.join(shorten(w) for w in main_words(col)),
            max_len, cnt)

def get_conflicts(aliases):
    '''
    >>> sorted(get_conflicts({'invoice': 'i', 'act': 'a', 'account': 'a', 'contract': 'c', 'consume': 'c', 'person': 'p'}))
    ['account', 'act', 'consume', 'contract']
    '''
    return sum(
        [tbls for tbls in
         [[k for k, v in aliases.items() if v == a]
          for a in set(aliases.values())]
         if len(tbls) > 1], [])

def mk_table_aliases(tbl_words, word_pos):
    '''
    >>> sorted(mk_table_aliases({'t_payment_method': ['pymnt', 'mtd'], \
                          't_contract': ['cntrct']}, \
                         {'t_payment_method': [2, 1], \
                          't_contract': [2]}).items())
    [('t_contract', 'cn'), ('t_payment_method', 'pym')]
    '''
    return { k: ''.join(w[:word_pos[k][i]]
                           for (i, w) in enumerate(tbl_words[k]))
                for k in tbl_words }

def indexof(x, arr):
    '''
    >>> indexof(1, [0, 3, 1, 4, 1])
    2
    >>> indexof(2, [2, 1, 3])
    0
    >>> str(indexof(3, [4, 5]))
    'None'
    '''
    pos = [i for (i, c) in enumerate(arr) if c == x]
    return pos[0] if pos else None

def fallback_table_aliases(confl_tbls, tbl_words, word_pos):
    '''
    >>> sorted(fallback_table_aliases( \
        ['t_longtable1', 't_longtable2'], \
        {'t_payment_method': ['pymnt', 'mtd'], \
         't_longtable2': ['lngtbl2'], \
         't_contract': ['cntrct'], \
         't_longtable1': ['lngtbl1'], \
        }, \
        {'t_payment_method': [2, 1], \
         't_contract': [2], \
         't_longtable1': [4], \
         't_longtable2': [4], \
        }).items())
    [('t_contract', 'cn'), ('t_longtable1', 'lngt1'), \
('t_longtable2', 'lngt2'), ('t_payment_method', 'pym')]
    '''
    confl_tbls = sorted(confl_tbls)
    return {k: (v if k not in confl_tbls
                else '%s%d' % (v, indexof(k, confl_tbls) + 1))
            for k, v in mk_table_aliases(tbl_words, word_pos).items()}

def inc_word_pos(confl_tbls, tbl_words, word_pos):
    for tbl in confl_tbls:
        for (i, w) in enumerate(tbl_words[tbl]):
            if word_pos[tbl][i] < len(w):
                word_pos[tbl][i] += 1
                break

def table_aliases(tbls):
    '''
    >>> sorted(table_aliases(('client', 'order')).items())
    [('client', 'c'), ('order', 'o')]
    >>> sorted(table_aliases(('t_paysys', 't_payment_method', 't_order')).items())
    [('t_order', 'o'), ('t_payment_method', 'pm'), ('t_paysys', 'p')]
    >>> sorted(table_aliases(('client', 'contract', 'order')).items())
    [('client', 'cl'), ('contract', 'cn'), ('order', 'o')]
    >>> sorted(table_aliases(('pomidoro', 't_payment_method', 'PayMethod')).items())
    [('PayMethod', 'pymt'), ('pomidoro', 'p'), ('t_payment_method', 'pymm')]
    >>> sorted(table_aliases(('abc', 'longtable1', \
            'longtable4', 'longtable3')).items())
    [('abc', 'a'), ('longtable1', 'lngtb1'), \
('longtable3', 'lngtb2'), ('longtable4', 'lngtb3')]
    >>> sorted(table_aliases(('abc', 'longt1', \
            'longt4', 'longt3')).items())
    [('abc', 'a'), ('longt1', 'lngt1'), \
('longt3', 'lngt3'), ('longt4', 'lngt4')]
    '''
    tbls = set(tbls)
    tbl_words = { k: [shorten(w) for w in main_words(k)]
                  for k in tbls }
    word_pos = { k: [ 1 for w in tbl_words[k] ]
                 for k in tbls }
    for i in range(5):
        if i:
            inc_word_pos(confl_tbls, tbl_words, word_pos)
        aliases = mk_table_aliases(tbl_words, word_pos)
        confl_tbls = get_conflicts(aliases)
        if not confl_tbls:
            return aliases
    return fallback_table_aliases(confl_tbls, tbl_words, word_pos)

def col_aliases(p, max_len):
    '''
    >>> col_aliases([('t_client', 'name'), \
            ('t_payment_method', 'paysys_id'), \
            ], 12)
    ['c_name', 'pm_paysys_id']
    >>> col_aliases([('t_paysys', 'name'), \
            ('t_passport', 'uid'), \
            ('t_payment_method', 'paysys_id'), \
            ], 11)
    ['py_name', 'ps_uid', 'pm_paysys_3']
    '''
    tbls = set(tbl for tbl, col in p)
    t_aliases = table_aliases(tbls)
    return [mk_alias(t_aliases[tbl], col, max_len, i + 1)
            for (i, (tbl, col)) in enumerate(p)]
    
if __name__ == '__main__':
    import doctest
    doctest.testmod()
