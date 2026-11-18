# Testy překladače IFJ26

```
tests/
  Makefile                      # make unit | integration | valgrind | valgrind-leaks | all | clean
  run_integration_tests.sh      # integrační testy (bash)
  unit/                         # unit testy (cmocka), jeden program na jeden modul
    test_helpers.{h,c}  test_scanner.c  test_symbol_table.c  test_ast.c
    test_parser.c       test_semantic.c test_codegen.c
  integration_test_codes/       # valid/ lexical_error/ syntax_error/ semantic_error/ runtime_error/
```

Složka `tests/` leží v kořeni projektu vedle `src/` a `ic26int`.

## Spuštění

```
sudo apt install libcmocka-dev valgrind
make -C tests unit            # 390 unit testů, překlad s -std=c11 -Wall -Wextra -Werror -pedantic
make -C tests valgrind        # neplatné čtení/zápis, neinicializované hodnoty
make -C tests integration     # 105 programů: ifj26 < x.ifj26 | ic26int
```

Integrační skript použije `./ifj26` a `./ic26int` z kořene projektu; když `ifj26` chybí, přeloží si
překladač sám do `tests/build/ifj26`. Cesty lze přepsat: `COMPILER=... INTERP=... ./run_integration_tests.sh [-v] [-k] [vzor]`.

Očekávané kódy: `valid` 0, `lexical_error` 1, `syntax_error` 2, `semantic_error` podle názvu
(`e3_…` až `e7_…`), `runtime_error` – překlad 0 a kód interpretu v `.run_rc` (40 / 57 / 58).

## Poznámky

- Testy se přeloží s oběma variantami scanneru. Pokud je v `scanner.h` typ `scanner_result_t`, Makefile
  přidá `-DSCANNER_HAS_RESULT` a lexikální chyba se čte z `conf.result`; jinak se čeká `exit(1)`.
- Každý test má timeout 5 s (SIGALRM), nekonečná smyčka v kódu tedy test jen shodí.
- `make valgrind-leaks` hlásí úniky v chybových větvích parseru (nepřijatý token se při chybě neuvolní).
