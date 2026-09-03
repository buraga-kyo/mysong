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

# ─── Nota do caso dos DOUS FIOS da bibliotheca (issue #69) ──────────────────
# O caso «ler a bibliotheca emquanto outro fio a reabre não a parte», em
# prova_biblioteca.cpp, é INERTE n'esta compilação: tirada a tranca da
# bibliotheca, as tres asserções d'elle continuam a passar, e a bateria fica
# verde com a corrida de pé. Quem o julga é o sanitizador de fios, e sómente
# elle. Correndo assim, e o -tc com o nome INTEIRO, que nome de caso não leva
# ponto e vírgula (parte a lista do CMake) nem vírgula (separa os filtros do
# doctest, e o binario salta tudo e sahe com successo):
#
#   setarch "$(uname -m)" -R cmake -B build-tsan -S . \
#     -DCMAKE_CXX_FLAGS='-fsanitize=thread -g -O1' \
#     -DCMAKE_EXE_LINKER_FLAGS='-fsanitize=thread'
#   setarch "$(uname -m)" -R cmake --build build-tsan --target mysong_testes
#   setarch "$(uname -m)" -R ./build-tsan/testes/mysong_testes \
#     -tc='ler a bibliotheca emquanto outro fio a reabre não a parte'
#
# O setarch é d'esta machina: sem elle o sanitizador morre no mapeamento.
#
# E medido: CINCO casos do prova_api_viva.cpp, da issue #4, já levam vírgula no
# nome. Pelo ctest correm todos, que elle cita o nome inteiro; pelo -tc á mão,
# pedem curinga («-tc='*byte a byte'»), senão o binario salta os 269 e sahe com
# successo.

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

# ─── A PROVA DA ONDA (issue #131) ───────────────────────────────────────────
# Ao lado da prova da capa, que é o cache de quem a onda toma a somma e a raiz.
# Linha NOVA, e nunca alteração da d'ella. O ffmpeg não corre aqui senão no
# caso VIVO, que dorme sem a variavel MYSONG_PROVA_MP3.
target_sources(mysong_testes PRIVATE prova_onda.cpp)

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

# ─── A PROVA DA SALA (issue #92) ────────────────────────────────────────────
# Ao lado da prova da tabella, que é a peça que a sala estende: linha NOVA.
target_sources(mysong_testes PRIVATE prova_sala.cpp)

# ─── A PROVA DO CORREIO (issue #12) ─────────────────────────────────────────
target_sources(mysong_testes PRIVATE prova_correio.cpp)

# ─── A PROVA DAS LISTAS (issue #10) ─────────────────────────────────────────
# Corre em directorio temporario: o caminho do banco entra por parâmetro.
target_sources(mysong_testes PRIVATE prova_rol.cpp)

# ─── A PROVA DA LIXEIRA (issue #105) ────────────────────────────────────────
# Ao lado da prova das listas, e pela mesma razão: a raiz entra por parâmetro,
# donde a bateria corre em temporario e não toca a lixeira de quem nos usa.
target_sources(mysong_testes PRIVATE prova_lixeira.cpp)

# ─── A PROVA DO PROJECTOR (issue #17) ───────────────────────────────────────
# Janella alguma se abre: o que se afere é o que se HA DE correr e o que se HA DE
# mandar. Dous casos erguem processo, e nenhum d'elles abre video.
target_sources(mysong_testes PRIVATE prova_video.cpp)

# ─── A PROVA DO CATALOGO (issue #13) ────────────────────────────────────────
# Sem rede: a pagina de embutir vae escripta á mão, com o embrulho e os objectos
# aninhados conservados, que são as duas cousas que um leitor plano erraria.
target_sources(mysong_testes PRIVATE prova_catalogo.cpp)

# ─── A PROVA DOS FIOS (issue #50) ───────────────────────────────────────────
# Dous e tres fios a bater no MESMO tocador, mil voltas cada. O aceite corre-a
# tambem sob -fsanitize=thread, por build dir proprio, e lê a saída limpa.
target_sources(mysong_testes PRIVATE prova_tocador_fios.cpp)

# ─── A PROVA DO MUSICBRAINZ (issue #57) ─────────────────────────────────────
# Sem rede: os corpos são recortes verbatim das respostas vivas de 2026-08-27.
# O unico caso que toca o mundo toca RELOGIO (o acelerador), e não rede.
target_sources(mysong_testes PRIVATE prova_musicbrainz.cpp)

# ─── A PROVA DO COVER ART ARCHIVE (issue #83) ───────────────────────────────
# Sem rede: a Consulta entra de mentira e as faixas são fixtures em temporario.
target_sources(mysong_testes PRIVATE prova_caa.cpp)

# ─── A PROVA DA LINHA DE COMMANDO (issue #66) ───────────────────────────────
target_sources(mysong_testes PRIVATE prova_linha.cpp)
# ─── A PROVA DOS AJUSTES (issue #67) ────────────────────────────────────────
target_sources(mysong_testes PRIVATE prova_ajustes.cpp)

# ─── Prova do prompt em linha propria (issue #79) ───────────────────────────
# Por target_sources, ACRESCENTANDO linha, que é a regra d'esta Casa.
target_sources(mysong_testes PRIVATE prova_prompt.cpp)

# ─── Prova da vigilia do desenho (issue #82): por target_sources, a regra ───
target_sources(mysong_testes PRIVATE prova_vigilia.cpp)

# ─── Prova do rato (issue #95): por target_sources, a regra ─────────────────
target_sources(mysong_testes PRIVATE prova_rato.cpp)

# ─── Prova da lousa (issue #103): por target_sources, a regra ───────────────
target_sources(mysong_testes PRIVATE prova_lousa.cpp)

# ─── Prova do foco e das setas (issue #107): por target_sources, a regra ────
# A geometria entra á mão, e não da sala: o que se afere é o SALTO, e sala que
# mudasse de repartição mudaria a prova sem mudar a taboada.
target_sources(mysong_testes PRIVATE prova_foco.cpp)

# ─── Prova do letreiro (issue #108): por target_sources, a regra ───────────
target_sources(mysong_testes PRIVATE prova_letreiro.cpp)

# ─── Prova do cabeçalho em abas (issue #102): por target_sources, a regra ────
target_sources(mysong_testes PRIVATE prova_cabecalho.cpp)

# ─── Prova do rio da letra (issue #109): por target_sources, a regra ─────────
# Sem terminal, sem som e sem relogio: o rio é funcção da POSIÇÃO, e a posição
# escreve-se á mão. A composição sobre o espectro prova-se em écran de papel.
target_sources(mysong_testes PRIVATE prova_letra_viva.cpp)

# ─── Prova do menu de contexto (issue #96): por target_sources, a regra ─────
target_sources(mysong_testes PRIVATE prova_menu_contexto.cpp)
# A bateria do HELP (issue #133), em linha propria pela mesma regra.
target_sources(mysong_testes PRIVATE prova_ajuda.cpp)
