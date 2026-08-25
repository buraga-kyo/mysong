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

# A bateria da sonda (#22). Entra por target_sources, e não na lista do
# add_executable acima, porque linha nova não collide onde linha alterada
# collidiria: é a regra d'esta Casa, e não noticia de uma onda de trabalho.
target_sources(mysong_testes PRIVATE prova_sonda.cpp prova_tela_requisitos.cpp)

include("${doctest_SOURCE_DIR}/scripts/cmake/doctest.cmake")
doctest_discover_tests(mysong_testes)

# ─── Provas do núcleo de reprodução (issue #3) ──────────────────────────────
# Por target_sources, e não na lista do add_executable, para ACRESCENTAR sem
# tocar linha alguma que outra lavra esteja a editar.
target_sources(mysong_testes PRIVATE prova_fila.cpp prova_tocador.cpp)

# ─── Provas da superfície de commando (issue #4) ─────────────────────────────
# Por target_sources, e não na lista do add_executable, pela mesma regra das
# lavras acima: ACRESCENTAR linha, e não alterar linha que a tarefa irmã esteja a
# editar. Linha nova não collide onde linha alterada collidiria.
target_sources(mysong_testes PRIVATE prova_api.cpp prova_api_viva.cpp)

# ══════════════════════════════════════════════════════════════════════════
# ─── Prova do espectro (issue #5) ───────────────────────────────────────────
# Por target_sources, e não na lista do add_executable: linha nova não collide.
# Esta prova NÃO pede PipeWire nem fftw3 nos seus caminhos de inclusão: a
# mathematica se prova por espectro.hpp, que declara o plano da fftw adiante, e
# o contracto por analisador.hpp, que esconde o PipeWire atraz de um punho.
target_sources(mysong_testes PRIVATE prova_espectro.cpp)

# ─── Provas da taboa da libmpv (issue #28) ──────────────────────────────────
# Por target_sources, ACRESCENTANDO linha, que é a regra d'esta Casa.
target_sources(mysong_testes PRIVATE prova_libmpv.cpp)
# O caminho do binario da tela chega por definição, e a dependencia garante que
# elle exista quando o caso correr: prova que lê binario ausente não se salta,
# accusa. É o binario, e não este arquivo, o oraculo de fóra.
add_dependencies(mysong_testes mysong)
target_compile_definitions(mysong_testes PRIVATE
  MYSONG_BINARIO="$<TARGET_FILE:mysong>")

# ─── Prova do DESENHO do espectro (issue #6) ─────────────────────────────────
# Por target_sources, e não na lista do add_executable: linha nova não collide.
# Chama-se prova_espectro_tui.cpp porque prova_espectro.cpp já é a prova do
# núcleo, e dous arquivos de egual nome não cabem no mesmo directorio.
# SEM PONTO E VÍRGULA em nome de caso algum lá dentro: o ponto e vírgula parte a
# lista do CMake, e zero casos corridos dá status de successo. Já succedeu.
target_sources(mysong_testes PRIVATE prova_espectro_tui.cpp)

# ─── A PROVA DO TRANSPORTE (issue #7) ───────────────────────────────────────
# As duas peças provaveis da barra de baixo: a composição e a taboada das teclas.
# A janella não entra: ella abre terminal, motor e som, e não se prova em bateria.
target_sources(mysong_testes PRIVATE prova_transporte.cpp prova_commando.cpp)

# ─── A PROVA DA VARREDURA (issue #34) ───────────────────────────────────────
target_sources(mysong_testes PRIVATE prova_varredura.cpp)

# ─── A PROVA DA NAVEGAÇÃO (issue #9) ────────────────────────────────────────
target_sources(mysong_testes PRIVATE prova_navegador.cpp)

# ─── A PROVA DA AQUISIÇÃO (issue #11) ───────────────────────────────────────
target_sources(mysong_testes PRIVATE prova_aquisicao.cpp)

# ─── A PROVA DA LETRA (issue #14) ───────────────────────────────────────────
target_sources(mysong_testes PRIVATE prova_letra.cpp)

# ─── A PROVA DA CAPA (issue #16) ────────────────────────────────────────────
target_sources(mysong_testes PRIVATE prova_capa.cpp)

# ─── A PROVA DO MPRIS (issue #29) ───────────────────────────────────────────
target_sources(mysong_testes PRIVATE prova_unidades.cpp)

#   Da lavra do eminente Doutor BRAGA US. — Braga Us ✒
# ══════════════════════════════════════════════════════════════════════════

# ─── Provas da bibliotheca do acervo (issue #8) ──────────────────────────────
# Por target_sources, e não na lista do add_executable, pela mesma regra das
# lavras acima: linha nova não collide onde linha alterada collidiria, e a
# mysong-28 com a mysong-6 lavram neste arquivo agora.
target_sources(mysong_testes PRIVATE prova_biblioteca.cpp)

# ─── A PROVA DO ESTALEIRO (issue #12) ───────────────────────────────────────
# Sem rede: a obra entra por parametro, e no logar d'ella vae uma que se SEGURA.
target_sources(mysong_testes PRIVATE prova_estaleiro.cpp)

# ─── A PROVA DA TABELLA (issue #12) ─────────────────────────────────────────
# Pinta em écran de PAPEL, e lê-o CELLA A CELLA: é o unico modo de aferir
# alinhamento de columna sem o olho de quem abriu o programa.
target_sources(mysong_testes PRIVATE prova_tabella.cpp)

# ─── A PROVA DO CORREIO (issue #12) ─────────────────────────────────────────
target_sources(mysong_testes PRIVATE prova_correio.cpp)

# ─── A PROVA DAS LISTAS (issue #10) ─────────────────────────────────────────
# Corre em directorio temporario: o caminho do banco entra por parâmetro.
target_sources(mysong_testes PRIVATE prova_rol.cpp)

# ─── A PROVA DO PROJECTOR (issue #17) ───────────────────────────────────────
# Janella alguma se abre: o que se afere é o que se HA DE correr e o que se HA DE
# mandar. Dous casos erguem processo, e nenhum d'elles abre video.
target_sources(mysong_testes PRIVATE prova_video.cpp)
