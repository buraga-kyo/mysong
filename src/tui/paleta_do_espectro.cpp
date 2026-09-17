#include "tui/paleta_do_espectro.hpp"

#include "tui/tokens.hpp"

namespace mysong::tui {

const PaletaDoEspectro& paleta_do_espectro() noexcept {
  static const PaletaDoEspectro paleta{
      tokens::v500, tokens::text_faint, tokens::glow_hot, tokens::glow_hot,
      tokens::data5, tokens::data3, tokens::data2};
  return paleta;
}

std::string_view tinta_do_registro_na_paleta(Registro registro) noexcept {
  const PaletaDoEspectro& paleta = paleta_do_espectro();
  switch (registro) {
    case Registro::Graves: return paleta.graves;
    case Registro::MediosGraves: return paleta.medios_graves;
    case Registro::MediosAgudos: return paleta.medios_agudos;
    case Registro::Agudos: return paleta.agudos;
  }
  return paleta.graves;
}

}  // namespace mysong::tui
