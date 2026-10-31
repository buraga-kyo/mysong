#pragma once
#include <ftxui/dom/elements.hpp>
#include "tui/rato.hpp"
#include "tui/transporte.hpp"
namespace mysong::tui {
bool usa_mp3(std::size_t largura, std::size_t altura);
ftxui::Element controles_do_mp3(const Retracto&, std::size_t largura, CaixasDoCabecalho* = nullptr);
ftxui::Element elemento_do_mp3(const Retracto&, std::size_t largura, std::size_t altura,
                              CaixasDoCabecalho* = nullptr);
}
