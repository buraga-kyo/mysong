#include "tui/orquestrador.hpp"
#include "api/socket.hpp"

namespace mysong::tui {

AppContext::AppContext(
    const mysong::nucleo::Ajustes& ajustes,
    const std::filesystem::path& banco,
    const std::filesystem::path& listas,
    const std::filesystem::path& soquete) {
  const std::filesystem::path acervo = ajustes.acervo.valor;
  std::string razao;
  std::error_code erro_da_pasta;
  std::filesystem::create_directories(acervo, erro_da_pasta);
  if (erro_da_pasta) {
    erro_fatal = "não foi possível abrir a biblioteca: " + erro_da_pasta.message();
    return;
  }
  
  std::optional<mysong::nucleo::MotorMpv> motor_temp = mysong::nucleo::MotorMpv::abrir(&razao);
  if (!motor_temp) {
    erro_fatal = "a machina de som não abriu: " + razao;
    return;
  }
  motor.emplace(std::move(*motor_temp));

  tocador = std::make_unique<mysong::nucleo::Tocador>(*motor);
  tocador->volume(ajustes.volume.valor);
  
  if (analisador.vivo()) {
    tocador->observa(analisador);
  } else {
    std::cerr << "mysong: sem espectro: " << analisador.razao() << "\n";
  }

  mpris = std::make_unique<mysong::api::CasaDoMpris>(*tocador);
  if (!mpris->viva()) {
    std::cerr << "mysong: sem MPRIS: " << mpris->razao() << "\n";
  }

  livraria = std::make_unique<mysong::nucleo::Biblioteca>(banco);
  roleiro = std::make_unique<mysong::nucleo::Roleiro>(listas, acervo);
  if (!roleiro->erro().empty()) std::cerr << "mysong: " << roleiro->erro() << "\n";
  projector = std::make_unique<mysong::nucleo::Projector>(soquete);
  navegador = std::make_unique<mysong::tui::Navegador>(*livraria, roleiro.get());

  estaleiro = std::make_unique<mysong::nucleo::Estaleiro>(
      ajustes.baixas_simultaneas.valor,
      [acervo](const mysong::nucleo::Pedido& pedido, std::filesystem::path* ficou) {
        return mysong::nucleo::baixa(acervo, pedido, ficou);
      });

  std::string razao_do_socket;
  arredores = std::make_unique<mysong::api::Arredores>(mysong::api::Arredores{livraria.get(), estaleiro.get()});
  
  std::optional<mysong::api::Servidor> servidor_temp = mysong::api::Servidor::abrir(
      *tocador, mysong::api::caminho_padrao_do_socket(), &razao_do_socket, *arredores);
  if (!servidor_temp) {
    std::cerr << "mysong: sem socket de commando: " << razao_do_socket << "\n";
  } else {
    servidor.emplace(std::move(*servidor_temp));
  }
}

} // namespace mysong::tui