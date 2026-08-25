# ══════════════════════════════════════════════════════════════════════════
#   TRACTADO DA OBRA — CMakeLists.txt
# ══════════════════════════════════════════════════════════════════════════
# DOMÍNIO ......... arvore limpa deste repositorio, e rede que responda na
#                   PRIMEIRA configuracao (dahi por diante, nao mais).
# CONTRA-DOMÍNIO .. os alvos mysong_core (bibliotheca do núcleo), mysong (o
#                   executavel) e mysong_testes (a bateria de provas).
# INVARIANTE ...... os avisos -Wall -Wextra incidem SO nos alvos desta Casa, por
#                   target_compile_options e nunca por flag global: terceiros
#                   entram como SYSTEM e calam-se. Toda dependencia de
#                   FetchContent vem presa a ETIQUETA fixa, jamais a um ramo.
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

add_library(mysong_core STATIC src/nucleo/marca.cpp src/tui/arrowline.cpp)
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

# ─── A ÂNCORA DO MOTOR (issue #28) ──────────────────────────────────────────
# Põe o motor DENTRO do binario da tela hoje, que é a configuração que a issue #7
# ha de criar. Sem ella, a prova da ligação passaria por ausencia de motor, e não
# por acerto. Vae por target_sources, ACRESCENTANDO linha, e sem tocar
# src/tui/janella.cpp, que é da #7. Some quando a #7 chamar o motor de verdade.
target_sources(mysong PRIVATE src/tui/ancora_do_motor.cpp)

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

# A FITA do espectro: a peça que o olho do operador julga, e a sahida cujos bytes
# se conferem com o grep. Linha de commando, e SEM FTXUI na conta que ella pinta:
# a fita resolve-se em quadro, e o quadro emitte-se em SGR cru. Alvo proprio, e
# não a janella: a janella é da issue #7, e esta onda não lhe toca.
add_executable(fita_espectro exemplos/fita_espectro.cpp)
target_compile_options(fita_espectro PRIVATE ${MYSONG_AVISOS})
target_link_libraries(fita_espectro PRIVATE mysong_core)

#   Da lavra do eminente Doutor BRAGA US. — Braga Us ✒
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
