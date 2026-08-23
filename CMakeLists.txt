# ══════════════════════════════════════════════════════════════════════════
#   TRACTADO DA OBRA — CMakeLists.txt
# ══════════════════════════════════════════════════════════════════════════
# DOMÍNIO ......... arvore limpa deste repositorio, e rede que responda na
#                   PRIMEIRA configuracao (dahi por diante, nao mais).
# CONTRA-DOMÍNIO .. os alvos mysong_core (bibliotheca do núcleo), mysong (o
#                   executavel) e mysong_testes (a bateria de provas).
# INVARIANTE ...... os avisos -Wall -Wextra incidem SO nos alvos desta Casa, por
#                   target_compile_options e nunca por flag global: terceiros
#                   entram como SYSTEM e calam-se. Toda dependencia vem presa a
#                   ETIQUETA fixa, jamais a um ramo.
# Q.E.D. .......... quem clona e roda os dois commandos do README obtem o mesmo
#                   binario que o auctor, sem pacote do systema a installar.
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
# ligação que já existe: tarefas irmãs mexem neste mesmo arquivo agora, e linha
# NOVA não collide onde linha alterada collidiria. Fontconfig é o unico pacote de
# systema que este alvo exige; o dl vem para o dlopen da libmpv, que se carrega
# em tempo de execução justamente para não morrer antes do main sem ella.
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

# ══════════════════════════════════════════════════════════════════════════
#   Da lavra do eminente Doutor BRAGA US. — Braga Us ✒
# ══════════════════════════════════════════════════════════════════════════
