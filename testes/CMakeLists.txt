# ══════════════════════════════════════════════════════════════════════════
#   TRACTADO DA BATERIA — testes/CMakeLists.txt
# ══════════════════════════════════════════════════════════════════════════
# DOMÍNIO ......... as fontes de prova deste directorio.
# CONTRA-DOMÍNIO .. o alvo mysong_testes, e um caso registrado no ctest para
#                   CADA caso do doctest, contado nominalmente.
# INVARIANTE ...... nenhuma prova existe sem estar registrada: o registro vem de
#                   doctest_discover_tests, que interroga o binario ja
#                   compilado, e nao de uma lista escripta a mao que envelhece.
# Q.E.D. .......... havendo registro por caso, `ctest` que nada encontre accusa
#                   contagem zero, e a falha silenciosa deixa de se esconder.
# ══════════════════════════════════════════════════════════════════════════
add_executable(mysong_testes fumo_marca.cpp prova_arrowline.cpp)
target_compile_options(mysong_testes PRIVATE ${MYSONG_AVISOS})
target_link_libraries(mysong_testes PRIVATE mysong_core doctest::doctest)

include("${doctest_SOURCE_DIR}/scripts/cmake/doctest.cmake")
doctest_discover_tests(mysong_testes)

# ─── Provas do núcleo de reprodução (issue #3) ──────────────────────────────
# Por target_sources, e não na lista do add_executable, para ACRESCENTAR sem
# tocar linha alguma que outra lavra esteja a editar.
target_sources(mysong_testes PRIVATE prova_fila.cpp)

# ══════════════════════════════════════════════════════════════════════════
#   Da lavra do eminente Doutor BRAGA US. — Braga Us ✒
# ══════════════════════════════════════════════════════════════════════════
