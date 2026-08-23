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
# Entra por target_sources, e não na lista do add_library: assim esta issue
# ACRESCENTA sem tocar linha alguma que outra lavra esteja a editar.
target_sources(mysong_core PRIVATE src/nucleo/fila.cpp)

# ══════════════════════════════════════════════════════════════════════════
#   Da lavra do eminente Doutor BRAGA US. — Braga Us ✒
# ══════════════════════════════════════════════════════════════════════════
