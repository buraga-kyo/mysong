#include "nucleo/canvas.hpp"
#include "api/jsonzinho.hpp"
#include <curl/curl.h>
#include <cstdint>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <memory>
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
struct Corpo { std::string texto; std::size_t limite; };
// § Recolhe octetos sob um tecto explicito; zero interrompe excesso ou alocação.
// O receptor CURL conserva o corpo vivo e nunca recebe excepção de C++.
std::size_t recolhe(char* dados, std::size_t tamanho, std::size_t quantos,
                    void* destino) {
  auto& corpo = *static_cast<Corpo*>(destino);
  if (tamanho && quantos > (corpo.limite - corpo.texto.size()) / tamanho) return 0;
  const auto bytes = tamanho * quantos;
  try { corpo.texto.append(dados, bytes); } catch (...) { return 0; }
  return bytes;
}
// § Consulta HTTPS em prazo finito, sem redirecção; credencial só ao destinatario.
// HTTP diverso de 200 ou falha de transporte devolve corpo vazio.
std::string consulta(const std::string& url, std::size_t limite,
                     const std::string& credencial = {}, const std::string& carga = {}) {
  std::unique_ptr<CURL, decltype(&curl_easy_cleanup)> punho(curl_easy_init(), curl_easy_cleanup);
  if (!punho) return {};
  Corpo corpo{{}, limite};
  auto* rede = punho.get();
  curl_easy_setopt(rede, CURLOPT_URL, url.c_str());
  curl_easy_setopt(rede, CURLOPT_WRITEFUNCTION, recolhe);
  curl_easy_setopt(rede, CURLOPT_WRITEDATA, &corpo);
  curl_easy_setopt(rede, CURLOPT_CONNECTTIMEOUT, 5L);
  curl_easy_setopt(rede, CURLOPT_TIMEOUT, 25L);
  curl_easy_setopt(rede, CURLOPT_NOSIGNAL, 1L);
  curl_easy_setopt(rede, CURLOPT_USERAGENT, "Mozilla/5.0 mysong/0.1");
  std::unique_ptr<curl_slist, decltype(&curl_slist_free_all)> cabecalho(
      curl_slist_append(nullptr, "Content-Type: application/x-protobuf"), curl_slist_free_all);
  if (!carga.empty()) {
    if (!cabecalho) return {};
    curl_easy_setopt(rede, CURLOPT_HTTPHEADER, cabecalho.get());
    curl_easy_setopt(rede, CURLOPT_HTTPAUTH, CURLAUTH_BEARER);
    curl_easy_setopt(rede, CURLOPT_XOAUTH2_BEARER, credencial.c_str());
    curl_easy_setopt(rede, CURLOPT_POSTFIELDS, carga.data());
    curl_easy_setopt(rede, CURLOPT_POSTFIELDSIZE, static_cast<long>(carga.size()));
  }
  const auto resultado = curl_easy_perform(rede);
  long estado = 0;
  curl_easy_getinfo(rede, CURLINFO_RESPONSE_CODE, &estado);
  return resultado == CURLE_OK && estado == 200 ? corpo.texto : std::string{};
}
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
// § Elege video da faixa exacta e da origem de media do Spotify.
// Não segue endereços alheios; texto com controles ou resposta invalida dá vazio.
std::string video_da_entidade(std::string_view corpo, std::string_view id) {
  std::string url, uri;
  std::uint64_t especie = 0, valor = 0;
  unsigned numero = 0, tipo = 0;
  std::string_view texto;
  while (!corpo.empty()) {
    if (!campo(corpo, numero, tipo, valor, texto)) return {};
    if (numero == 2 && tipo == 2) url = std::string(texto);
    if (numero == 4 && tipo == 0) especie = valor;
    if (numero == 5 && tipo == 2) uri = std::string(texto);
  }
  if (uri != "spotify:track:" + std::string(id) || especie < 1 || especie > 3 ||
      url.rfind("https://canvaz.scdn.co/", 0) != 0 || url.size() > 4096) return {};
  for (const unsigned char letra : url)
    if (letra <= 32 || letra == 127 || letra == '\\') return {};
  return url;
}
}  // namespace detalhe_canvas
// § Examina toda a mensagem antes de aceitar a URL; cauda mutilada a invalida.
// Sem effeitos; vazio significa que não houve video elegivel na resposta.
std::string url_do_canvas(std::string_view resposta, std::string_view id) {
  if (pedido_do_canvas(id).empty() || resposta.size() > 1024 * 1024) return {};
  std::string url;
  unsigned numero = 0, tipo = 0;
  std::uint64_t valor = 0;
  std::string_view texto;
  while (!resposta.empty()) {
    if (!detalhe_canvas::campo(resposta, numero, tipo, valor, texto)) return {};
    if (numero == 1 && tipo == 2) {
      const auto candidata = detalhe_canvas::video_da_entidade(texto, id);
      if (url.empty()) url = candidata;
    }
  }
  return url;
}
// § Resolve URL publica; só altera o pedido após conferir URI, titulo e artista.
// Sem credenciais persistidas; rede ou pagina incompleta devolvem falso.
bool resolve_faixa_spotify(Pedido* pedido) {
  if (pedido == nullptr) return false;
  const auto id = id_da_faixa_spotify(pedido->url);
  if (id.empty()) return false;
  const auto pagina = detalhe_canvas::consulta(
      "https://open.spotify.com/embed/track/" + id, 2 * 1024 * 1024);
  const auto entidade = api::recorta_objecto(pagina, "entity");
  if (api::texto_de_chave(entidade, "uri") != "spotify:track:" + id) return false;
  const auto titulo = api::texto_de_chave(entidade, "title");
  std::string artista;
  for (const auto& pessoa : api::objectos_do_arranjo(api::recorta_arranjo(entidade, "artists"))) {
    const auto nome = api::texto_de_chave(pessoa, "name");
    if (nome.empty()) continue;
    if (!artista.empty()) artista += ", ";
    artista += nome;
  }
  if (titulo.empty() || artista.empty()) return false;
  if (pedido->titulo.empty()) pedido->titulo = titulo;
  if (pedido->artista.empty()) pedido->artista = artista;
  double duracao = 0;
  if (pedido->duracao == 0 && api::numero_de_chave(entidade, "duration", &duracao) &&
      duracao > 0 && duracao <= 86400000) pedido->duracao = static_cast<int>(duracao / 1000);
  pedido->id_spotify = id;
  pedido->fonte = Fonte::Spotify;
  pedido->url.clear();
  return true;
}
// § Destino separado do audio, com ID para distinguir versões homonymas.
// Sem effeitos; ID invalido dá caminho vazio, nunca um nome vindo da rede.
std::filesystem::path destino_do_canvas(const std::filesystem::path& raiz,
                                        const Pedido& pedido) {
  if (pedido_do_canvas(pedido.id_spotify).empty()) return {};
  std::string titulo = saneia_nome(pedido.titulo);
  if (titulo.size() > 180) {
    std::size_t fronteira = 180;
    while ((static_cast<unsigned char>(titulo[fronteira]) & 0xc0) == 0x80) --fronteira;
    titulo.resize(fronteira);
  }
  return raiz / "Artistas" / saneia_nome(pedido.artista) / "Clipes" / "Canvas" /
      (titulo + " [" + pedido.id_spotify + "].mp4");
}
// § Publica sómente MP4 inteiro; temporario exclusivo impede colisão entre fios.
// Falha apaga só o temporario proprio; ligação atomica jámais substitue destino.
ColheitaCanvas grava_canvas(const std::filesystem::path& destino, std::string_view corpo) {
  if (destino.empty() || corpo.size() < 12 || corpo.substr(4, 4) != "ftyp")
    return ColheitaCanvas::Falhou;
  std::error_code erro;
  std::filesystem::create_directories(destino.parent_path(), erro);
  if (erro) return ColheitaCanvas::Falhou;
  std::string temporario = destino.string() + ".XXXXXX";
  const int descritor = ::mkstemp(temporario.data());
  if (descritor < 0) return ColheitaCanvas::Falhou;
  FILE* arquivo = ::fdopen(descritor, "wb");
  if (!arquivo) { ::close(descritor); ::unlink(temporario.c_str()); return ColheitaCanvas::Falhou; }
  bool pronto = std::fwrite(corpo.data(), 1, corpo.size(), arquivo) == corpo.size();
  if (std::fflush(arquivo) != 0 || ::fsync(descritor) != 0) pronto = false;
  if (std::fclose(arquivo) != 0) pronto = false;
  ColheitaCanvas resultado = ColheitaCanvas::Falhou;
  if (pronto) {
    if (::link(temporario.c_str(), destino.c_str()) == 0) resultado = ColheitaCanvas::Gravado;
    else if (errno == EEXIST) resultado = ColheitaCanvas::JaExiste;
  }
  ::unlink(temporario.c_str());
  return resultado;
}
// § Tenta o video sem comprometter o audio; prazos e tectos pertencem á consulta.
// Sessão publica é a omissão; credencial opcional vive só na memoria do processo.
ColheitaCanvas baixa_canvas(const std::filesystem::path& raiz, const Pedido& pedido) {
  const auto destino = destino_do_canvas(raiz, pedido);
  if (destino.empty()) return ColheitaCanvas::Indisponivel;
  std::error_code erro;
  if (std::filesystem::is_regular_file(destino, erro)) return ColheitaCanvas::JaExiste;
  std::string credencial;
  const char* configurada = std::getenv("MYSONG_SPOTIFY_TOKEN");
  if (configurada && *configurada) credencial = configurada;
  else {
    const auto pagina = detalhe_canvas::consulta(
        "https://open.spotify.com/embed/track/" + pedido.id_spotify, 2 * 1024 * 1024);
    credencial = api::texto_de_chave(api::recorta_objecto(pagina, "session"), "accessToken");
  }
  if (credencial.empty()) return ColheitaCanvas::Falhou;
  const auto resposta = detalhe_canvas::consulta(
      "https://spclient.wg.spotify.com/canvaz-cache/v0/canvases", 1024 * 1024,
      credencial, pedido_do_canvas(pedido.id_spotify));
  if (resposta.empty()) return ColheitaCanvas::Falhou;
  const auto url = url_do_canvas(resposta, pedido.id_spotify);
  if (url.empty()) return ColheitaCanvas::Indisponivel;
  return grava_canvas(destino, detalhe_canvas::consulta(url, 24 * 1024 * 1024));
}
}  // namespace mysong::nucleo
