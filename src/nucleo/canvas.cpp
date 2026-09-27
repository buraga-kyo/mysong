#include "nucleo/canvas.hpp"
#include "api/jsonzinho.hpp"
#include <curl/curl.h>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <unistd.h>

namespace mysong::nucleo {
// § Reconhece sómente a origem declarada; ID tem 22 algarismos de base 62.
// Sem effeitos: dominio estranho, caminho excedente ou ID mutilado dão vazio.
std::string id_da_faixa_spotify(std::string_view entrada) {
  for (const std::string_view prefixo : {"https://open.spotify.com/track/",
       "https://open.spotify.com/embed/track/", "spotify:track:"}) {
    if (entrada.substr(0, prefixo.size()) != prefixo) continue;
    const auto resto = entrada.substr(prefixo.size());
    const auto id = resto.substr(0, resto.find_first_of("?#"));
    if (id.size() != 22) return {};
    for (const char letra : id)
      if (!((letra >= 'a' && letra <= 'z') || (letra >= 'A' && letra <= 'Z') ||
            (letra >= '0' && letra <= '9'))) return {};
    return std::string(id);
  }
  return {};
}
// § Escreve o pedido protobuf só depois de provar o ID; sem estado exterior.
// Os comprimentos cabem n'um octeto, pois a URI mede invariavelmente 36.
std::string pedido_do_canvas(std::string_view id) {
  if (id_da_faixa_spotify("spotify:track:" + std::string(id)) != id || id.empty())
    return {};
  const std::string uri = "spotify:track:" + std::string(id);
  return std::string("\x0a\x26\x0a\x24", 4) + uri;
}
namespace detalhe_canvas {
// § Lê inteiro sem sinal de até 64 bits, consumindo sómente octetos existentes.
// Falso denuncia truncamento ou transbordo; o deslocamento nunca excede 63.
bool inteiro(std::string_view& corpo, std::uint64_t& valor) {
  valor = 0;
  for (unsigned deslocamento = 0; deslocamento < 64; deslocamento += 7) {
    if (corpo.empty()) return false;
    const auto octeto = static_cast<unsigned char>(corpo.front());
    corpo.remove_prefix(1);
    if (deslocamento == 63 && octeto > 1) return false;
    valor |= std::uint64_t(octeto & 127) << deslocamento;
    if ((octeto & 128) == 0) return true;
  }
  return false;
}
// § Separa um campo protobuf; desconhecidos de tamanho fixo são consumidos.
// Entrada mutilada dá falso; vistas apontam para o corpo vivo do chamador.
bool campo(std::string_view& corpo, unsigned& numero, unsigned& tipo,
           std::uint64_t& valor, std::string_view& texto) {
  std::uint64_t chave = 0;
  texto = {};
  if (!inteiro(corpo, chave) || chave < 8 || chave >> 3 > 0x1fffffff) return false;
  numero = static_cast<unsigned>(chave >> 3);
  tipo = static_cast<unsigned>(chave & 7);
  if (tipo == 0) return inteiro(corpo, valor);
  if (tipo == 2) {
    if (!inteiro(corpo, valor) || valor > corpo.size()) return false;
  } else if (tipo == 1 || tipo == 5) {
    valor = tipo == 1 ? 8 : 4;
    if (valor > corpo.size()) return false;
  } else return false;
  texto = corpo.substr(0, static_cast<std::size_t>(valor));
  corpo.remove_prefix(texto.size());
  return true;
}
}  // namespace detalhe_canvas
}  // namespace mysong::nucleo
