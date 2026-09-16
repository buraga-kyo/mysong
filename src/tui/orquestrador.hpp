#pragma once
#include "nucleo/ajustes.hpp"
#include "nucleo/motor.hpp"
#include "nucleo/tocador.hpp"
#include "nucleo/analisador.hpp"
#include "api/mpris.hpp"
#include "api/socket.hpp"
#include "nucleo/biblioteca.hpp"
#include "nucleo/rol.hpp"
#include "nucleo/video.hpp"
#include "tui/navegador.hpp"
#include "nucleo/estaleiro.hpp"
#include "api/protocolo.hpp"
#include <memory>
#include <optional>
#include <string>
#include <filesystem>
#include <iostream>

namespace mysong::tui {

// AppContext: centraliza a orquestração e instanciação do núcleo.
struct AppContext {
  std::optional<std::string> erro_fatal;
  std::optional<mysong::nucleo::MotorMpv> motor;
  std::unique_ptr<mysong::nucleo::Tocador> tocador;
  mysong::nucleo::Analisador analisador;
  std::unique_ptr<mysong::api::CasaDoMpris> mpris;
  std::unique_ptr<mysong::nucleo::Biblioteca> livraria;
  std::unique_ptr<mysong::nucleo::Roleiro> roleiro;
  std::unique_ptr<mysong::nucleo::Projector> projector;
  std::unique_ptr<mysong::tui::Navegador> navegador;
  std::unique_ptr<mysong::nucleo::Estaleiro> estaleiro;
  std::optional<mysong::api::Servidor> servidor;
  std::unique_ptr<mysong::api::Arredores> arredores;

  explicit AppContext(
      const mysong::nucleo::Ajustes& ajustes,
      const std::filesystem::path& banco,
      const std::filesystem::path& listas,
      const std::filesystem::path& soquete);
};

} // namespace mysong::tui