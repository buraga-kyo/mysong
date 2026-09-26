# ══════════════════════════════════════════════════════════════════════════
#   TRACTADO DA OBRA, CMakeLists.txt
# ══════════════════════════════════════════════════════════════════════════
# DOMÍNIO ......... arvore limpa deste repositorio, e rede que responda na
#                   PRIMEIRA configuracao (dahi por diante, nao mais).
# CONTRA-DOMÍNIO .. os alvos mysong_core (bibliotheca do núcleo), mysong (o
#                   executavel) e mysong_testes (a bateria de provas).
# INVARIANTE ...... os avisos -Wall -Wextra incidem SO nos alvos desta Casa, por
#                   target_compile_options e nunca por flag global: terceiros
#                   entram como SYSTEM e calam-se. E o aviso RECUSA: a opcao
#                   MYSONG_WERROR traz o -Werror e vem ligada. Toda dependencia
#                   de FetchContent vem presa a ETIQUETA fixa, jamais a um ramo.
# Q.E.D. .......... quem clona e roda os dois commandos do README obtem o mesmo
#                   binario que o auctor. Duas classes de dependencia, e a
#                   fronteira e firme: as de INTERFACE e de PROVA (FTXUI,
#                   doctest) vem por FetchContent em etiqueta fixa e pacote
#                   algum pedem ao systema; as de SYSTEMA vem por pkg-config e
#                   pedem, e sao a libmpv e o fontconfig, mais o que a obra
#                   vier a cobrar. A taboada do README diz quaes, e de que
#                   pacote do apt cada uma sahe.
# ══════════════════════════════════════════════════════════════════════════
cmake_minimum_required(VERSION 3.25)
project(mysong VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(MYSONG_AVISOS -Wall -Wextra)  # avisos desta Casa, num só lugar

# ─── O GATE DA COMPILAÇÃO (issue #64) ───────────────────────────────────────
# O aviso desta Casa passa a valer por RECUSA. Até aqui o -Wall -Wextra pedia e
# não cobrava: quem esquecesse um ramo de switch novo via um aviso que a rolagem
# come, e a guarda de compilação era política humana em vez de gate.
#
# Entra na LISTA acima, e nunca em CMAKE_CXX_FLAGS. A lista só existe expandida
# dentro do target_compile_options de cada alvo d'esta Casa, e por isso a
# bandeira sahe NOS ALVOS: o FTXUI e o doctest vem por FetchContent, e regra
# d'esta Casa não julga obra alheia. Alvo novo que siga o costume herda o gate
# sem que ninguém se lembre d'elle, que é o que dez linhas repetidas não dão.
#
# LIGADA por omissão. A opção não existe para se desligar no dia a dia: existe
# para o dia em que compilador ou versão nova traga aviso inédito, que nesse dia
# a obra compila-se com -DMYSONG_WERROR=OFF em vez de ficar sem se compilar.
option(MYSONG_WERROR "O aviso desta Casa vale por erro" ON)
if(MYSONG_WERROR)
  list(APPEND MYSONG_AVISOS -Werror)
endif()

include(FetchContent)

# FTXUI: a janella. Nao existe no apt; vem presa a etiqueta, nunca a um ramo.
foreach(opcao FTXUI_BUILD_EXAMPLES FTXUI_BUILD_DOCS FTXUI_BUILD_TESTS
              FTXUI_ENABLE_INSTALL)
  set(${opcao} OFF CACHE BOOL "" FORCE)
endforeach()
FetchContent_Declare(ftxui
  GIT_REPOSITORY https://github.com/ArthurSonzogni/FTXUI
  GIT_TAG v7.0.3
  GIT_SHALLOW TRUE
  SYSTEM)
FetchContent_MakeAvailable(ftxui)

# Um build que já tinha o FTXUI baixado não passa novamente pelo PATCH_COMMAND.
# Reaplica-se a mesma emenda na configuração quando a fonte ainda traz o
# agendamento periódico, sem tocar uma vez sequer numa árvore já corrigida.
set(MYSONG_FTXUI_APP "${ftxui_SOURCE_DIR}/src/ftxui/component/app.cpp")
if(EXISTS "${MYSONG_FTXUI_APP}")
  file(READ "${MYSONG_FTXUI_APP}" MYSONG_FTXUI_APP_TEXT)
  string(FIND "${MYSONG_FTXUI_APP_TEXT}" "  PostAnimationTask();\n\n  installed_" MYSONG_FTXUI_PERIODIC_CALL)
  if(NOT MYSONG_FTXUI_PERIODIC_CALL EQUAL -1)
    execute_process(
      COMMAND git -C "${ftxui_SOURCE_DIR}" apply
              "${CMAKE_CURRENT_SOURCE_DIR}/cmake/ftxui-no-periodic-redraw.patch"
      RESULT_VARIABLE MYSONG_FTXUI_PATCH_RESULT)
    if(NOT MYSONG_FTXUI_PATCH_RESULT EQUAL 0)
      message(FATAL_ERROR "Não consegui aplicar a emenda de redraw do FTXUI")
    endif()
    execute_process(
      COMMAND python3 "${CMAKE_CURRENT_SOURCE_DIR}/cmake/patch_ftxui_sync.py" "${MYSONG_FTXUI_APP}"
    )
  endif()
endif()

add_library(mysong_core STATIC src/nucleo/marca.cpp src/tui/arrowline.cpp src/tui/orquestrador.cpp)
target_include_directories(mysong_core PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_compile_options(mysong_core PRIVATE ${MYSONG_AVISOS})

add_executable(mysong src/tui/janella.cpp)
target_compile_options(mysong PRIVATE ${MYSONG_AVISOS})
target_link_libraries(mysong PRIVATE mysong_core
  ftxui::component ftxui::dom ftxui::screen)

# Exemplo da fita arrowline: a peça que o olho do operador ha de julgar.
add_executable(fita_arrowline exemplos/fita_arrowline.cpp)
target_compile_options(fita_arrowline PRIVATE ${MYSONG_AVISOS})
target_link_libraries(fita_arrowline PRIVATE mysong_core)

# ── A SONDA dos requisitos. Acrescenta-se por target_sources e por uma segunda
# chamada de target_link_libraries, e NÃO na linha do add_library nem na lista de
# ligação que já existe: é regra d'esta Casa que lavra nova ACRESCENTE linha em
# vez de alterar linha alheia, porque linha nova não collide onde linha alterada
# collidiria. D'esta lavra vem o fontconfig, com que a sonda acha a fonte; e vem
# o dl, com que ella pergunta por dlopen pela presença da libmpv.
find_package(PkgConfig REQUIRED)
pkg_check_modules(FONTCONFIG REQUIRED IMPORTED_TARGET fontconfig)
target_sources(mysong_core PRIVATE src/nucleo/sonda.cpp src/tui/tela_requisitos.cpp)
target_link_libraries(mysong_core PRIVATE PkgConfig::FONTCONFIG ${CMAKE_DL_LIBS})
# A tela dos requisitos pinta com o FTXUI, e por isso o nucleo passa a ligá-lo em
# PUBLIC: é o que permitte á bateria pintar o quadro FÓRA de terminal e aferir o
# que elle diz, em vez de o julgar pelo olho de quem o abriu.
target_link_libraries(mysong_core PUBLIC ftxui::dom ftxui::screen)

# ─── O QUE O DOCTEST NÃO HA DE INSTALLAR (issue #66) ────────────────────────
# Vae ANTES do MakeAvailable, e é a unica lavra da #66 fóra do fim do arquivo:
# o doctest declara DOCTEST_NO_INSTALL como opção DESLIGADA, e entrando elle
# por subdirectorio as sete chamadas de install() d'elle passariam a ser
# nossas, e ellas dão SEIS regras de file(INSTALL) no script gerado, que é o
# numero que eu medi tirando esta linha: quem installasse o mysong levaria
# junto os cabeçalhos, o pacote de CMake e o de pkg-config de uma bibliotheca
# de PROVA. Depois do MakeAvailable já não ha como as desfazer, e por isso
# esta linha não pôde ir com as outras.
set(DOCTEST_NO_INSTALL ON CACHE BOOL "" FORCE)

# doctest: a bateria. Cabecalho unico, presa a etiqueta como a janella.
FetchContent_Declare(doctest
  GIT_REPOSITORY https://github.com/doctest/doctest
  GIT_TAG v2.5.3
  GIT_SHALLOW TRUE
  SYSTEM)
FetchContent_MakeAvailable(doctest)

enable_testing()
add_subdirectory(testes)

# ─── O núcleo de reprodução (issue #3) ──────────────────────────────────────
# Entra por target_sources, e não na lista do add_library, pela mesma regra da
# sonda: ACRESCENTAR linha, e não alterar linha que outra lavra possa editar.
target_sources(mysong_core PRIVATE src/nucleo/fila.cpp src/nucleo/tocador.cpp)

# A libmpv vem do systema, achada por pkg-config e NUNCA por caminho chumbado.
# Entra como SYSTEM para que os seus cabecalhos se calem sob -Wall -Wextra, do
# mesmo modo que a FTXUI e o doctest.
#
# E entra SÓ POR CABEÇALHO. A ligação sahiu na issue #28: binario que LIGA a
# libmpv morre no carregador dynamico antes do main quando ella falta, e a sonda
# dos requisitos nunca corre para dizer o que falta. Quem a chama é a taboa de
# src/nucleo/libmpv.cpp, por dlopen. A linha da ligação foi EDITADA, e não
# neutralizada por linha nova em outro logar: duas linhas em briga fariam este
# arquivo mentir sobre a ligação, que é o defeito que a #28 veio corrigir.
find_package(PkgConfig REQUIRED)
pkg_check_modules(MPV REQUIRED IMPORTED_TARGET mpv)
target_sources(mysong_core PRIVATE src/nucleo/motor.cpp)
target_include_directories(mysong_core SYSTEM PUBLIC ${MPV_INCLUDE_DIRS})

# A prova de som: linha de commando, e SEM FTXUI. Que não linke a janella é
# parte do aceite, e por isso se lê aqui e se confirma com ldd.
add_executable(toca_tom exemplos/toca_tom.cpp)
target_compile_options(toca_tom PRIVATE ${MYSONG_AVISOS})
target_link_libraries(toca_tom PRIVATE mysong_core)

# ─── A SUPERFÍCIE DE COMMANDO (issue #4) ────────────────────────────────────
# Entra por target_sources, e NÃO na lista do add_library nem em linha que já
# existisse: é regra d'esta Casa que lavra nova ACRESCENTE linha em vez de alterar
# linha alheia, e aqui a regra tem nome, que a tarefa irmã mysong-5 edita este
# mesmo arquivo agora. Dependencia nova alguma se cobra: o socket Unix é POSIX, e
# o jsonzinho é só-cabeçalho d'esta Casa. A metade MPRIS d'esta issue sahiu para a
# #29, e é ella que trará a libdbus-1.
target_sources(mysong_core PRIVATE src/api/protocolo.cpp src/api/socket.cpp)

# O binario com que o aceite se prova. Linha de commando, e SEM FTXUI: que ele NÃO
# linke a janella é parte do aceite, e por isso se lê aqui e se confirma com ldd.
add_executable(superficie_viva exemplos/superficie_viva.cpp)
target_compile_options(superficie_viva PRIVATE ${MYSONG_AVISOS})
target_link_libraries(superficie_viva PRIVATE mysong_core)

# ══════════════════════════════════════════════════════════════════════════
# ─── O ESPECTRO (issue #5) ──────────────────────────────────────────────────
# Duas dependencias de SYSTEMA novas, por pkg-config: libpipewire-0.3 (o cliente
# do grafo) e fftw3f (a transformada em precisão simples, que é a das amostras;
# pedir a dupla seria converter de graça). Entram como SYSTEM, pela propriedade
# INTERFACE_SYSTEM_INCLUDE_DIRECTORIES: os cabeçalhos do spa são muitos e
# falantes, e é a palavra SYSTEM que os cala sob -Wall -Wextra e guarda o nosso
# zero aviso. ACRESCENTA linha, e não altera linha alguma: a #4 lavra ao lado.
find_package(PkgConfig REQUIRED)
pkg_check_modules(PIPEWIRE REQUIRED IMPORTED_TARGET libpipewire-0.3)
pkg_check_modules(FFTW3F REQUIRED IMPORTED_TARGET fftw3f)
set_property(TARGET PkgConfig::PIPEWIRE APPEND PROPERTY
  INTERFACE_SYSTEM_INCLUDE_DIRECTORIES ${PIPEWIRE_INCLUDE_DIRS})
target_sources(mysong_core PRIVATE src/nucleo/espectro.cpp src/nucleo/analisador.cpp)
target_link_libraries(mysong_core PRIVATE PkgConfig::PIPEWIRE PkgConfig::FFTW3F)

# O ESPIA das bandas: o instrumento do aceite. Imprime as bandas em texto, com
# carimbo de tempo, para que a prova da NEGATIVA se leia linha por linha.
add_executable(espia_bandas exemplos/espia_bandas.cpp)
target_compile_options(espia_bandas PRIVATE ${MYSONG_AVISOS})
target_link_libraries(espia_bandas PRIVATE mysong_core)

# ─── A TABOA DA LIBMPV (issue #28) ──────────────────────────────────────────
# Entra por target_sources, ACRESCENTANDO linha: é a regra d'esta Casa. Pede
# directorio de inclusão algum, que o mpv/client.h mora em /usr/include.
target_sources(mysong_core PRIVATE src/nucleo/libmpv.cpp)

# ─── O DESENHO DO ESPECTRO (issue #6) ───────────────────────────────────────
# Por target_sources, e NÃO na lista do add_library nem em linha alheia: regra
# d'esta Casa, e aqui com nome, que as tarefas irmãs mysong-28 e mysong-8 lavram
# este mesmo arquivo agora. Dependencia nova alguma se cobra: o desenho é conta
# inteira mais os tokens, e o FTXUI o nucleo já liga em PUBLIC desde a sonda.
#
# NOTA DO HOMONYMO: esta fonte chama-se espectro.cpp como a de src/nucleo/, e as
# duas entram no MESMO alvo. Não collidem porque o CMake dispõe os objectos pelo
# caminho RELATIVO da fonte, e não pelo nome nú; uma colhe o espectro, a outra
# o desenha, e o nome é o certo em ambas.
target_sources(mysong_core PRIVATE src/tui/espectro.cpp)
target_sources(mysong_core PRIVATE src/tui/dinamica_do_espectro.cpp
  src/tui/paleta_do_espectro.cpp)
target_sources(mysong_core PRIVATE src/tui/geometria_da_janella.cpp)

# ─── A ONDA NA TELA (issue #131) ────────────────────────────────────────────
# A peça pura que pinta a onda da faixa n'uma linha da fita. Linha NOVA ao
# lado da do espectro, que é a peça de quem ella toma a escada dos oito
# blocos; nada se altera na d'elle, que a issue #132 lavra alli agora.
target_sources(mysong_core PRIVATE src/tui/onda.cpp)

# A FITA do espectro: a peça que o olho do operador julga, e a sahida cujos bytes
# se conferem com o grep. Linha de commando, e SEM FTXUI na conta que ella pinta:
# a fita resolve-se em quadro, e o quadro emitte-se em SGR cru. Alvo proprio, e
# não a janella: a janella é da issue #7, e esta onda não lhe toca.
add_executable(fita_espectro exemplos/fita_espectro.cpp)
target_compile_options(fita_espectro PRIVATE ${MYSONG_AVISOS})
target_link_libraries(fita_espectro PRIVATE mysong_core)

# A FITA da capa (issue #94): a mesma ideia, para a arte do album. Pinta uma
# imagem com os argumentos que o painel usa, e o terceiro argumento força o
# sextante, de sorte que o operador compare os dous renders no seu terminal.
# ACRESCENTA linha, ao lado do irmão, e não altera linha alguma.
add_executable(fita_capa exemplos/fita_capa.cpp)
target_compile_options(fita_capa PRIVATE ${MYSONG_AVISOS})
target_link_libraries(fita_capa PRIVATE mysong_core)

# ─── A BARRA DE TRANSPORTE (issue #7) ───────────────────────────────────────
# As duas peças provaveis da tela: a composição, que não conhece Tocador, e a
# taboada das teclas, que não toca no tocador. Entram no nucleo por
# target_sources, ACRESCENTANDO linha, como manda a regra d'esta Casa.
target_sources(mysong_core PRIVATE src/tui/transporte.cpp src/tui/commando.cpp)

# E o nucleo passa a ligar ftxui::component, que é onde `ftxui::Event` mora.
# ACRESCENTA-SE em linha propria, e não se edita a linha 70, que é lavra alheia.
#
# A escolha é deliberada, e a alternativa foi pesada: podia-se inventar um tipo
# de tecla proprio e mapear `ftxui::Event` para elle em janella.cpp. Mas então o
# mapeamento moraria na UNICA camada que não se prova, e é justamente o
# mapeamento que o criterio C6 quer aferir. Prova-se pois contra o evento REAL, e
# o preço é esta linha. Vale notar que a issue #6 já pôs src/tui/ no nucleo com
# ftxui::dom; esta linha segue o precedente, e não abre porta nova.
target_link_libraries(mysong_core PUBLIC ftxui::component)

# ─── A VARREDURA DO ACERVO (issue #34) ──────────────────────────────────────
# Entra por target_sources, ACRESCENTANDO linha. Dependencia nova alguma se
# cobra: a taglib e o sqlite3 já entraram com a bibliotheca na issue #8.
target_sources(mysong_core PRIVATE src/nucleo/varredura.cpp src/nucleo/renomeacao.cpp)

# A prova do esquema mais novo ENVELHECE a Casa por SQL propria, e para isso pede
# os cabeçalhos do sqlite3. A alternativa era pôr na obra um parametro de versão
# que sómente a prova usaria, e isso seria uma porta por onde alguem escreveria a
# versão errada de verdade. Sómente CABEÇALHOS: os symbolos já vêm do nucleo, que
# liga o sqlite3, e a linha vae aqui e não em testes/ porque é aqui que o
# pkg_check_modules já correu.
target_include_directories(mysong_testes SYSTEM PRIVATE ${SQLITE3_INCLUDE_DIRS})

# ─── A NAVEGAÇÃO DA BIBLIOTHECA (issue #9) ──────────────────────────────────
target_sources(mysong_core PRIVATE src/tui/navegador.cpp src/tui/tabella.cpp)

# ─── A SALA EM TRES PAINEIS (issue #92) ─────────────────────────────────────
# Por target_sources, ACRESCENTANDO linha ao lado da tabella que ella estende.
target_sources(mysong_core PRIVATE src/tui/sala.cpp)

# ─── A AQUISIÇÃO (issue #11) ────────────────────────────────────────────────
# A unica camada que toca a rede. Não pede dependencia nova: chama o yt-dlp por
# fork e exec, e escreve a etiqueta com a taglib, que já entrou na issue #8.
target_sources(mysong_core PRIVATE src/nucleo/aquisicao.cpp)

# ─── A LETRA DO LRCLIB (issue #14) ──────────────────────────────────────────
# Pede o libcurl, que é a segunda e ultima porta de rede d'esta obra: a primeira é
# o yt-dlp, que corre por fork. Entra como SYSTEM, como as demais alheias.
find_package(PkgConfig REQUIRED)
pkg_check_modules(CURL REQUIRED IMPORTED_TARGET libcurl)
target_sources(mysong_core PRIVATE src/nucleo/letra.cpp)
target_link_libraries(mysong_core PRIVATE PkgConfig::CURL)

# ─── A CAPA DO ALBUM (issue #16) ────────────────────────────────────────────
# Dependencia nova alguma: a taglib já entrou na #8, e o chafa corre por fork, como
# o yt-dlp da #11.
target_sources(mysong_core PRIVATE src/nucleo/capa.cpp)

# ─── A ONDA DA FAIXA (issue #131) ───────────────────────────────────────────
# Dependencia nova alguma se cobra: o ffmpeg corre por fork, como o chafa da
# #16 e o yt-dlp da #11, e o cache é o mesmo da capa. Linha PROPRIA ao lado
# d'ella, e não alteração da d'ella: a issue #132 lavra neste arquivo agora, e
# linha nova não collide onde linha alterada collidiria.
target_sources(mysong_core PRIVATE src/nucleo/onda.cpp)

# ─── O MPRIS (issue #29) ────────────────────────────────────────────────────
# As unidades vão em arquivo PROPRIO e sem libdbus, para se provarem em machina surda.
# O `grep dbus src/api/unidades.cpp` sahe vazio, e é o que faz a promessa verificavel.
target_sources(mysong_core PRIVATE src/api/unidades.cpp)

# E o encanamento, que pede a libdbus-1. Entra como SYSTEM, como as demais alheias.
pkg_check_modules(DBUS REQUIRED IMPORTED_TARGET dbus-1)
target_sources(mysong_core PRIVATE src/api/mpris.cpp)
target_link_libraries(mysong_core PRIVATE PkgConfig::DBUS)

#   Da lavra do eminente Doutor BRAGA US., Braga Us ✒
# ══════════════════════════════════════════════════════════════════════════

# ─── A BIBLIOTHECA DO ACERVO (issue #8) ─────────────────────────────────────
# Duas dependencias de SYSTEMA novas, por pkg-config e nunca por caminho
# chumbado: a taglib, que lê as etiquetas, e o sqlite3, que guarda o índice.
# ACRESCENTA linha, e NÃO altera linha alguma: a mysong-28 lavra em motor.* e
# sonda.*, e a mysong-6 em src/tui/espectro.*, nos mesmos arquivos de construcção
# agora, e linha nova não collide onde linha alterada collidiria.
#
# A palavra SYSTEM não é enfeite. A taglib não publica os seus cabeçalhos como
# SYSTEM por si, e elles são muitos: sem a propriedade
# INTERFACE_SYSTEM_INCLUDE_DIRECTORIES, sujariam o nosso zero aviso com avisos
# que não são nossos, e o zero aviso d'esta Casa deixaria de valer como sinal.
find_package(PkgConfig REQUIRED)
pkg_check_modules(TAGLIB REQUIRED IMPORTED_TARGET taglib)
pkg_check_modules(SQLITE3 REQUIRED IMPORTED_TARGET sqlite3)
set_property(TARGET PkgConfig::TAGLIB APPEND PROPERTY
  INTERFACE_SYSTEM_INCLUDE_DIRECTORIES ${TAGLIB_INCLUDE_DIRS})
target_sources(mysong_core PRIVATE src/nucleo/biblioteca.cpp)
target_link_libraries(mysong_core PRIVATE PkgConfig::SQLITE3 PkgConfig::TAGLIB)

# ─── O ESTALEIRO DAS BAIXAS (issue #12) ─────────────────────────────────────
# A fila com limite declarado. Dependencia nova alguma se cobra: a obra entra por
# std::function, e quem a cumpre é a aquisição da #11, que já está no alvo.
#
# E o Threads, que até aqui vinha de graça: o std::thread da janella ligava por a
# glibc de hoje trazer o pthread dentro de si. Pedir-lhe o nome torna a ligação
# DITA em vez de sorteada, e é o que faz esta obra compilar em machina cuja
# bibliotheca de C ainda separe as duas.
find_package(Threads REQUIRED)
target_sources(mysong_core PRIVATE src/nucleo/estaleiro.cpp)
target_link_libraries(mysong_core PUBLIC Threads::Threads)

# ─── O CORREIO DOS ACHADOS (issue #12) ──────────────────────────────────────
# Sómente-cabeçalho desde a issue #13, que o fez gabarito no que carrega: unidade de
# traducção alguma se compila, e por isso linha alguma se acrescenta aqui.

# ─── AS LISTAS DO OPERADOR (issue #10) ──────────────────────────────────────
# Banco PROPRIO, e não o da bibliotheca: aquelle é reconstruido a cada varredura
# por temporario e rename, e taboa de lista lá dentro sahiria com a varredura.
# Dependencia nova alguma: o sqlite3 já entrou com a bibliotheca na issue #8.
target_sources(mysong_core PRIVATE src/nucleo/rol.cpp src/nucleo/espelho.cpp)

# ─── A LIXEIRA DO SYSTEMA (issue #105) ──────────────────────────────────────
# Apagar faixa é mandá-la á lixeira freedesktop, e nunca desligá-la do disco.
# Dependencia nova alguma se cobra: o que a especificação pede é directorio,
# arquivo de texto e rename, que o <filesystem> da norma já dá.
target_sources(mysong_core PRIVATE src/nucleo/lixeira.cpp)

# ─── O VÍDEO EM JANELLA PROPRIA (issue #17) ─────────────────────────────────
# Segunda instancia de mpv, em processo proprio, governada por soquete de commando.
# Dependencia de bibliotheca nova alguma: o soquete é POSIX, e o mpv corre por
# fork e exec, como o yt-dlp da issue #11 e o chafa da #16.
target_sources(mysong_core PRIVATE src/nucleo/video.cpp)

# ─── O CATALOGO DO SPOTIFY (issue #13) ──────────────────────────────────────
# Lê a pagina publica de embutir por libcurl, que entrou com a letra na issue #14.
# Dependencia nova alguma se cobra, e o audio continua a vir do yt-dlp da #11.
target_sources(mysong_core PRIVATE src/nucleo/catalogo.cpp)

# ─── A GRAVAÇÃO NO MUSICBRAINZ (issue #57) ──────────────────────────────────
# Resolve a faixa do catalogo em gravação: ISRC, album, anno e numero canonicos.
# Rede pelo MESMO libcurl do catalogo; dependencia nova alguma se cobra.
target_sources(mysong_core PRIVATE src/nucleo/musicbrainz.cpp)

# ─── A CAPA DO ACERVO PELO COVER ART ARCHIVE (issue #83) ────────────────────
# A caça da capa que falta ao que já está no disco: casa no MB, pede ao CAA,
# embute. Rede pelo MESMO consulta_mb da #57; dependencia nova alguma se cobra.
target_sources(mysong_core PRIVATE src/nucleo/caa.cpp)

# O ESPIA do catalogo: o instrumento com que a leitura se afere contra a pagina de
# verdade, antes de o corpo entrar na bateria escripto á mão.
add_executable(espia_catalogo exemplos/espia_catalogo.cpp)
target_compile_options(espia_catalogo PRIVATE ${MYSONG_AVISOS})
target_link_libraries(espia_catalogo PRIVATE mysong_core)

# O ESPIA do MusicBrainz: a prova viva da resolução da issue #57, passo a passo.
add_executable(espia_musicbrainz exemplos/espia_musicbrainz.cpp)
target_compile_options(espia_musicbrainz PRIVATE ${MYSONG_AVISOS})
target_link_libraries(espia_musicbrainz PRIVATE mysong_core)

# O ESPIA da capa: a prova viva da caça da issue #83, sobre faixa de rascunho.
add_executable(espia_capa exemplos/espia_capa.cpp)
target_compile_options(espia_capa PRIVATE ${MYSONG_AVISOS})
target_link_libraries(espia_capa PRIVATE mysong_core)

# ─── O CURL GLOBAL (issue #63) ──────────────────────────────────────────────
# O main ergue o libcurl uma vez, antes de fio algum, e por isso o alvo da tela
# passa a pedir o cabeçalho do curl: o nucleo liga-o em PRIVATE, donde os
# caminhos de inclusão não lhe chegavam. ACRESCENTA linha, como manda a regra.
target_link_libraries(mysong PRIVATE PkgConfig::CURL)
# ─── O NUMERO DA VERSÃO (issue #66) ─────────────────────────────────────────
# O numero mora n'um logar só, que é o project() lá em cima, e chega ao C++ por
# cabeçalho CONFIGURADO. Constante copiada no codigo envelheceria á parte, e a
# obra passaria a mentir sobre a sua propria versão no dia em que o numero
# subisse. Vae para a arvore de compilação, que é onde cousa gerada mora.
configure_file(src/nucleo/versao.hpp.in
  "${CMAKE_CURRENT_BINARY_DIR}/gerado/nucleo/versao.hpp" @ONLY)
target_include_directories(mysong_core PUBLIC "${CMAKE_CURRENT_BINARY_DIR}/gerado")

# ─── A LEITURA DA LINHA DE COMMANDO (issue #66) ─────────────────────────────
# Entra por target_sources, ACRESCENTANDO linha, que é a regra d'esta Casa.
target_sources(mysong_core PRIVATE src/nucleo/linha.cpp)

# ─── A INSTALLAÇÃO (issue #66) ──────────────────────────────────────────────
# Os destinos vêm do GNUInstallDirs, e NUNCA de caminho escripto á mão: sendo
# relativos, o prefixo continua a ser o que o operador der, e o DESTDIR de quem
# empacota funcciona sem se pedir cousa alguma á obra.
#
# Installa-se o BINARIO, e nada mais do que esta arvore produz: a bibliotheca
# do núcleo, os espias e a bateria são ferramenta de dentro d'esta Casa, e o
# systema de quem toca musica não tem que as carregar.
include(GNUInstallDirs)
install(TARGETS mysong RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")

# ─── A PAGINA DE MANUAL (issue #66) ─────────────────────────────────────────
# Escripta á mão em troff, e ferramenta nova alguma se cobra por ella. O
# configure_file não a gera: substitue-lhe o numero na linha do .TH, para que a
# versão que o manual annuncia não envelheça á parte da do binario.
configure_file(installacao/mysong.1.in
  "${CMAKE_CURRENT_BINARY_DIR}/gerado/mysong.1" @ONLY)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/gerado/mysong.1"
  DESTINATION "${CMAKE_INSTALL_MANDIR}/man1")

# ─── A ENTRADA DE MENU (issue #66) ──────────────────────────────────────────
# A obra é de terminal, e a entrada leva Terminal=true: sem essa chave, a area
# de trabalho ergueria o binario sem terminal algum, e o operador veria uma
# janella piscar e morrer. Icone não vae: a obra não traz nenhum, e chave Icon
# apontando para o que não existe dá quadricula vazia no menu, que é peor.
#
# O Exec leva o caminho ABSOLUTO, e não o nome nú: a sessão graphica não herda
# o PATH do shell de login em boa parte dos ambientes, e a entrada nasceria
# morta justamente na installação que o README recommenda, a de ~/.local. O
# TryExec não serve de remedio, que elle resolve contra o MESMO PATH: trocaria
# entrada morta por entrada ausente, que é peor de diagnosticar por não deixar
# rasto. O preço é o prefixo se fixar no CONFIGURE, e o README diz como.
configure_file(installacao/mysong.desktop.in
  "${CMAKE_CURRENT_BINARY_DIR}/gerado/mysong.desktop" @ONLY)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/gerado/mysong.desktop"
  DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/applications")
# ─── OS AJUSTES DO OPERADOR (issue #67) ─────────────────────────────────────
# Por target_sources, ACRESCENTANDO linha, que é a regra d'esta Casa. Dependencia
# nova alguma se cobra: o arquivo lê-se por ifstream.
target_sources(mysong_core PRIVATE src/nucleo/ajustes.cpp)

# ─── O PROMPT EM LINHA PROPRIA (issue #79) ──────────────────────────────────
# Por target_sources, ACRESCENTANDO linha. A peça é nova de proposito: o topo da
# tela precisava de se provar, e a janella não se prova.
target_sources(mysong_core PRIVATE src/tui/prompt.cpp)

# ─── O CABEÇALHO EM ABAS (issue #102) ───────────────────────────────────────
# Por target_sources, ACRESCENTANDO linha, que é a regra d'esta Casa. Peça nova
# pela razão do prompt e do rato: a linha do alto precisava de se provar cella
# a cella, e a janella não se prova.
target_sources(mysong_core PRIVATE src/tui/cabecalho.cpp)

# ─── O FOCO E AS SETAS (issue #107) ─────────────────────────────────────────
# Por target_sources, ACRESCENTANDO linha, que é a regra d'esta Casa. Peça nova
# pela razão do rato: o salto de peça em peça é geometria pura, e a janella,
# que é quem a consome, não se prova.
target_sources(mysong_core PRIVATE src/tui/foco.cpp)

# ─── O RATO (issue #95) ─────────────────────────────────────────────────────
# Por target_sources, ACRESCENTANDO linha, que é a regra d'esta Casa. Peça nova
# pela razão do prompt e do menu: a traducção de coordenada em gesto precisava
# de se provar, e a janella não se prova.
target_sources(mysong_core PRIVATE src/tui/rato.cpp)

# ─── A LOUSA DO ÜBERZUG++ (issue #103) ──────────────────────────────────────
# Por target_sources, ACRESCENTANDO linha, que é a regra d'esta Casa.
# Dependencia nova alguma se cobra: o `ueberzugpp` é PROGRAMA que se chama, e
# não bibliotheca que se linka; falta d'elle não impede a obra de compilar nem
# de correr, que a capa cae nos symbolos do chafa.
target_sources(mysong_core PRIVATE src/nucleo/lousa.cpp)

# ─── O LETREIRO EM XIROD (issue #108) ───────────────────────────────────────
# Por target_sources, ACRESCENTANDO linha, que é a regra d'esta Casa. Vae ao
# lado da lousa por d'ella depender: a chapa que este rasteriza é ella quem põe.
target_sources(mysong_core PRIVATE src/nucleo/letreiro.cpp)

# A FITA da lousa: a prova do OLHO sem se abrir o tocador. Alvo proprio, pelo
# molde do fita_capa; não corre em machina de integração, que ella não tem X11.
add_executable(fita_lousa exemplos/fita_lousa.cpp)
target_compile_options(fita_lousa PRIVATE ${MYSONG_AVISOS})
target_link_libraries(fita_lousa PRIVATE mysong_core)

# ─── O RIO DA LETRA (issue #109) ────────────────────────────────────────────
# Por target_sources, ACRESCENTANDO linha, que é a regra d'esta Casa. Peça nova
# pela razão do espectro: o rio resolve-se em QUADRO, e quadro prova-se sem
# terminal nem relogio; a janella não se prova.
target_sources(mysong_core PRIVATE src/tui/letra_viva.cpp)

# A FITA da letra (issue #109): o rio por cima do espectro, para o olho julgar
# sem se abrir o tocador. Alvo proprio, pelo molde do fita_espectro.
add_executable(fita_letra exemplos/fita_letra.cpp)
target_compile_options(fita_letra PRIVATE ${MYSONG_AVISOS})
target_link_libraries(fita_letra PRIVATE mysong_core)

# ─── O MENU DE CONTEXTO (issue #96) ─────────────────────────────────────────
# Por target_sources, ACRESCENTANDO linha, que é a regra d'esta Casa. Peça nova
# pela razão do rato: o estado do menu e a taboada de dentro d'elle precisam de
# se provar cella a cella, e a janella não se prova.
target_sources(mysong_core PRIVATE src/tui/menu_contexto.cpp)
# O HELP (issue #133): a janella da ajuda, em linha propria pela mesma regra.
target_sources(mysong_core PRIVATE src/tui/ajuda.cpp)
