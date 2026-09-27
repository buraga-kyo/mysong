#include <doctest/doctest.h>
#include "nucleo/canvas.hpp"
#include <fstream>
#include <unistd.h>
namespace nu = mysong::nucleo;
namespace {
const std::string identificador = "3OHfY25tqY28d16oZczHc8";
const std::string endereco = "https://canvaz.scdn.co/a.mp4";
// § Mensagem pequena escrita pelo contracto protobuf, sem consultar a rede.
std::string resposta_canvas(const std::string& url = endereco,
                           const std::string& id = identificador, char tipo = 2) {
  const std::string uri = "spotify:track:" + id;
  const auto entidade = std::string("\x12") + char(url.size()) + url +
      char(0x20) + tipo + char(0x2a) + char(uri.size()) + uri;
  return std::string("\x0a") + char(entidade.size()) + entidade;
}
}
// § Só video da faixa pedida e da origem declarada atravessa a fronteira.
TEST_CASE("Canvas confere a entidade e a origem") {
  CHECK(nu::url_do_canvas(resposta_canvas(), identificador) == endereco);
  CHECK(nu::url_do_canvas(resposta_canvas(endereco, std::string(22, 'a')), identificador).empty());
  CHECK(nu::url_do_canvas(resposta_canvas("https://canvaz.scdn.co.evil/a"), identificador).empty());
  CHECK(nu::url_do_canvas(resposta_canvas(endereco, identificador, 0), identificador).empty());
  CHECK(nu::url_do_canvas(resposta_canvas("https://canvaz.scdn.co/a\n"), identificador).empty());
}
