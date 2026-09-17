#pragma once

#include <string_view>

#include "tui/espectro.hpp"

namespace mysong::tui {

struct PaletaDoEspectro {
  std::string_view base;
  std::string_view silencio;
  std::string_view batida;
  std::string_view graves;
  std::string_view medios_graves;
  std::string_view medios_agudos;
  std::string_view agudos;
};

const PaletaDoEspectro& paleta_do_espectro() noexcept;
std::string_view tinta_do_registro_na_paleta(Registro registro) noexcept;

}  // namespace mysong::tui
