// ══════════════════════════════════════════════════════════════════════════
//   TRACTADO DA LETRA, src/nucleo/letra.cpp
// ══════════════════════════════════════════════════════════════════════════
// A implementação. As puras primeiro; o libcurl no fim, e sozinho.
//
// DOMÍNIO ......... o que se sabe da faixa, e o corpo do LRCLIB.
// CONTRA-DOMÍNIO .. um `.lrc` ao lado do audio, ou nada.
// INVARIANTE ...... funcção alguma d'aqui lança, e letra ausente não é falha.
// Q.E.D. .......... a rede toca-se n'uma funcção só, e por isso a bateria julga
//                   tudo o mais sobre corpos escriptos á mão.
// ══════════════════════════════════════════════════════════════════════════
#include "nucleo/letra.hpp"
#include "nucleo/versao.hpp"

#include <curl/curl.h>
#include "nucleo/cancelamento.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <iterator>
#include <cstdlib>
#include <cstdio>
#include <fstream>
#include <memory>
#include <mutex>
#include <thread>
#include <chrono>
#include <limits>
#include <ctime>
#include <cerrno>

#include "api/jsonzinho.hpp"

namespace mysong::nucleo {

// § Iguala caixa ASCII e espaços, conservando os demais octetos dos nomes.
// Sem effeitos exteriores; vazio continua vazio para não inventar metadado.
std::string nome_para_letra(std::string_view nome) {
  std::string resultado;
  bool espaco = false;
  for (const unsigned char letra : nome) {
    if (letra == ' ' || letra == '\t' || letra == '\n' || letra == '\r') {
      espaco = !resultado.empty();
      continue;
    }
    if (espaco) resultado += ' ';
    espaco = false;
    resultado += static_cast<char>(letra >= 'A' && letra <= 'Z' ? letra + 32 : letra);
  }
  return resultado;
}

std::string titulo_para_letra(std::string_view artista, std::string_view titulo) {
  const auto apara = [](std::string_view valor) {
    const auto inicio = valor.find_first_not_of(" \t\r\n");
    if (inicio == std::string_view::npos) return std::string{};
    return std::string(valor.substr(inicio, valor.find_last_not_of(" \t\r\n") - inicio + 1));
  };
  const auto autor = apara(artista);
  const auto nome = apara(titulo);
  if (!autor.empty() && nome.size() > autor.size() &&
      nome_para_letra(std::string_view(nome).substr(0, autor.size())) == nome_para_letra(autor)) {
    const auto resto = std::string_view(nome).substr(autor.size());
    for (const auto separador : {" - ", ": ", " \xe2\x80\x93 ", " \xe2\x80\x94 "}) {
      const std::string_view corte(separador);
      if (resto.substr(0, corte.size()) == corte && resto.size() > corte.size())
        return apara(resto.substr(corte.size()));
    }
  }
  return nome;
}

std::string agente_da_letra() {
  return "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 "
      "(KHTML, like Gecko) Chrome/131.0.0.0 Safari/537.36 MySong/" + std::string(versao()) +
      " (https://github.com/buraga-kyo/mysong)";
}
std::vector<std::string> cabecalhos_da_letra() {
  return {"Accept: application/json", "Accept-Language: pt-BR,pt;q=0.9,en;q=0.8",
      "Lrclib-Client: MySong/" + std::string(versao()) + " (https://github.com/buraga-kyo/mysong)"};
}

std::string escapa_para_url(std::string_view crua) {
  static const char kCifras[] = "0123456789ABCDEF";
  std::string obra;
  obra.reserve(crua.size() * 3);
  for (const unsigned char octeto : crua) {
    // A lista do que NÃO se escapa é a do RFC 3986 para «unreserved», e é
    // fechada: escapar de mais é sempre seguro, e escapar de menos parte a
    // consulta no primeiro `&` que um titulo traga.
    const bool livre = std::isalnum(octeto) != 0 || octeto == '-' ||
                       octeto == '.' || octeto == '_' || octeto == '~';
    if (livre) {
      obra += static_cast<char>(octeto);
      continue;
    }
    obra += '%';
    obra += kCifras[octeto >> 4];
    obra += kCifras[octeto & 0x0F];
  }
  return obra;
}

std::string url_da_busca(std::string_view artista, std::string_view titulo) {
  std::string url = "https://lrclib.net/api/search?track_name=";
  url += escapa_para_url(titulo_para_letra(artista, titulo));
  if (!artista.empty()) {
    url += "&artist_name=";
    url += escapa_para_url(artista);
  }
  return url;
}

std::string primeiro_objecto(std::string_view arranjo) {
  // O RECORTE mudou de casa na issue #13: vive em api/jsonzinho.hpp, que é onde o
  // JSON mora. Tinha-o aqui, e o catalogo precisaria de outro egual; duas cópias da
  // mesma conta dão duas verdades, e a que se corrigisse deixava a outra a errar.
  const std::vector<std::string> todos = api::objectos_do_arranjo(arranjo);
  return todos.empty() ? std::string() : todos.front();
}

std::filesystem::path caminho_do_lrc(const std::filesystem::path& audio) {
  std::filesystem::path lrc = audio;
  lrc.replace_extension(".lrc");
  return lrc;
}

Letra le_resposta(std::string_view corpo) {
  Letra letra;
  const std::string objecto = primeiro_objecto(corpo);
  if (objecto.empty()) return letra;
  // O jsonzinho d'esta Casa é o mesmo que o socket da issue #4 usa: objecto PLANO
  // de um nivel, com valores escalares. É exactamente a fórma que o LRCLIB dá.
  const api::Mensagem lida = api::analysa(objecto);
  if (!lida.valida) return letra;
  const api::Valor* sincronizada = lida.acha("syncedLyrics");
  const api::Valor* plana = lida.acha("plainLyrics");
  if (sincronizada != nullptr &&
      sincronizada->typo == api::Typo::Texto)
    letra.sincronizada = sincronizada->texto;
  if (plana != nullptr && plana->typo == api::Typo::Texto)
    letra.plana = plana->texto;
  return letra;
}

// § Elege o candidato temporalmente mais proximo da mesma faixa e artista.
// Sem duração comprovada conserva só a letra plana; a recusa fica observavel.
Letra escolhe_letra(std::string_view corpo, std::string_view artista,
                    std::string_view titulo, double duracao) {
  Letra eleita;
  double melhor = TOLERANCIA_DA_LETRA + 1;
  const auto nome = nome_para_letra(titulo_para_letra(artista, titulo)), autor = nome_para_letra(artista);
  if (nome.empty() || autor.empty()) return eleita;
  for (const auto& objeto : api::objectos_do_arranjo(corpo)) {
    if (nome_para_letra(api::texto_de_chave(objeto, "trackName")) != nome ||
        nome_para_letra(api::texto_de_chave(objeto, "artistName")) != autor) continue;
    const auto candidata = le_resposta("[" + objeto + "]");
    if (eleita.plana.empty()) eleita.plana = candidata.plana;
    if (candidata.sincronizada.empty()) continue;
    double segundos = 0;
    const bool medida = std::isfinite(duracao) && duracao > 0 &&
        api::numero_de_chave(objeto, "duration", &segundos) &&
        std::isfinite(segundos) && segundos > 0;
    const double distancia = std::abs(segundos - duracao);
    if (!medida || distancia > TOLERANCIA_DA_LETRA) {
      eleita.sincronizada_recusada = true;
    } else if (distancia < melhor) {
      melhor = distancia;
      eleita.sincronizada = candidata.sincronizada;
    }
  }
  if (!eleita.sincronizada.empty()) eleita.sincronizada_recusada = false;
  return eleita;
}
bool grava_lrc(const std::filesystem::path& audio, const Letra& letra) {
  const bool temporizada = !letra.sincronizada.empty();
  const auto& texto = temporizada ? letra.sincronizada : letra.plana;
  if (texto.empty()) return false;
  std::filesystem::path onde = caminho_do_lrc(audio);
  if (!temporizada) onde.replace_extension(".letra.txt");
  std::ofstream sahida(onde, std::ios::binary | std::ios::trunc);
  if (!sahida) return false;
  sahida << texto;
  if (texto.back() != '\n') sahida << '\n';
  return sahida.good();
}

namespace {

// recolhe, o que o libcurl entrega, pedaço a pedaço. Assignatura fixada por elle.
std::size_t recolhe(char* pedaco, std::size_t largura, std::size_t quantos,
                    void* alvo) {
  if (quantos != 0 && largura > std::numeric_limits<std::size_t>::max() / quantos) return 0;
  const std::size_t medida = largura * quantos;
  auto& corpo = *static_cast<std::string*>(alvo);
  constexpr std::size_t limite = 4 * 1024 * 1024;
  if (medida > limite || corpo.size() > limite - medida) return 0;
  corpo.append(pedaco, medida);
  return medida;
}

std::size_t recolhe_cabecalho(char* bytes, std::size_t largura, std::size_t quantos, void* alvo) {
  if (quantos != 0 && largura > std::numeric_limits<std::size_t>::max() / quantos) return 0;
  const auto tamanho = largura * quantos;
  const std::string_view linha(bytes, tamanho);
  auto& espera = *static_cast<std::string*>(alvo);
  if (linha.substr(0, 5) == "HTTP/") espera.clear();
  const auto separa = linha.find(':');
  if (separa != std::string_view::npos && nome_para_letra(linha.substr(0, separa)) == "retry-after") {
    const auto valor = linha.substr(separa + 1);
    const auto inicio = valor.find_first_not_of(" \t\r\n");
    if (inicio != std::string_view::npos)
      espera = std::string(valor.substr(inicio, valor.find_last_not_of(" \t\r\n") - inicio + 1));
  }
  return tamanho;
}

}  // namespace

bool interpreta_busca_da_letra(long estado, std::string_view corpo,
    std::string_view artista, std::string_view titulo, double duracao,
    Letra* letra, std::string* razao) {
  if (letra) *letra = {};
  if (razao) razao->clear();
  const auto recusa = [&](const std::string& motivo) {
    if (razao) *razao = motivo;
    return false;
  };
  if (estado < 200 || estado >= 300) {
    const std::string contexto = estado == 403 ? "acesso recusado" :
        estado == 429 ? "limite de consultas atingido" :
        estado >= 500 ? "serviço indisponível" : "resposta recusada";
    return recusa("LRCLIB: " + contexto + " (HTTP " + std::to_string(estado) + ")");
  }
  const auto inicio = corpo.find_first_not_of(" \t\r\n");
  const auto fim = corpo.find_last_not_of(" \t\r\n");
  if (inicio == std::string_view::npos || corpo[inicio] != '[' || corpo[fim] != ']')
    return recusa("LRCLIB: resposta inválida; era esperado um catálogo JSON");
  const auto eleita = escolhe_letra(corpo, artista, titulo, duracao);
  if (letra) *letra = eleita;
  if (razao && eleita.sincronizada.empty() && eleita.plana.empty())
    *razao = "LRCLIB: nenhuma letra compatível com o artista e o título";
  return true;
}

bool busca_letra(std::string_view artista, std::string_view titulo,
                 Letra* letra, double duracao, std::string* razao) {
  if (letra) *letra = {};
  if (razao) razao->clear();
  const auto recusa = [&](const std::string& motivo) {
    if (razao) *razao = motivo;
    return false;
  };
  if (titulo.empty()) return recusa("Letra: título não informado");
  // Duas baixas não disputam a API; espera e pausa respeitam o cancelamento.
  static std::timed_mutex porta;
  static auto anterior = std::chrono::steady_clock::time_point::min();
  static auto bloqueado_ate = std::chrono::steady_clock::time_point::min();
  std::unique_lock<std::timed_mutex> chave(porta, std::defer_lock);
  while (!chave.try_lock_for(std::chrono::milliseconds(50)))
    if (baixa_interrompida()) return recusa("Consulta da letra cancelada");
  if (std::chrono::steady_clock::now() < bloqueado_ate) {
    const auto segundos = std::chrono::duration_cast<std::chrono::seconds>(
        bloqueado_ate - std::chrono::steady_clock::now()).count() + 1;
    return recusa("LRCLIB: limite de consultas; aguarde " + std::to_string(segundos) + " segundos");
  }
  if (anterior != std::chrono::steady_clock::time_point::min())
    while (std::chrono::steady_clock::now() < anterior + std::chrono::milliseconds(250)) {
      if (baixa_interrompida()) return recusa("Consulta da letra cancelada");
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
  if (baixa_interrompida()) return recusa("Consulta da letra cancelada");
  std::unique_ptr<CURL, decltype(&curl_easy_cleanup)> punho(curl_easy_init(), curl_easy_cleanup);
  if (!punho) return recusa("Não foi possível iniciar a consulta da letra");
  std::unique_ptr<curl_slist, decltype(&curl_slist_free_all)> cabecalhos(nullptr, curl_slist_free_all);
  for (const auto& valor : cabecalhos_da_letra()) {
    auto* nova = curl_slist_append(cabecalhos.get(), valor.c_str());
    if (!nova) return recusa("Não foi possível preparar os cabeçalhos da letra");
    cabecalhos.release();
    cabecalhos.reset(nova);
  }
  const auto url = url_da_busca(artista, titulo);
  const auto agente = agente_da_letra();
  std::string corpo, espera_pedida;
  curl_easy_setopt(punho.get(), CURLOPT_URL, url.c_str());
  curl_easy_setopt(punho.get(), CURLOPT_HTTPHEADER, cabecalhos.get());
  curl_easy_setopt(punho.get(), CURLOPT_HEADERFUNCTION, recolhe_cabecalho);
  curl_easy_setopt(punho.get(), CURLOPT_HEADERDATA, &espera_pedida);
  curl_easy_setopt(punho.get(), CURLOPT_USERAGENT, agente.c_str());
  curl_easy_setopt(punho.get(), CURLOPT_ACCEPT_ENCODING, "");
  curl_easy_setopt(punho.get(), CURLOPT_WRITEFUNCTION, recolhe);
  curl_easy_setopt(punho.get(), CURLOPT_WRITEDATA, &corpo);
  curl_easy_setopt(punho.get(), CURLOPT_FOLLOWLOCATION, 1L);
  curl_easy_setopt(punho.get(), CURLOPT_MAXREDIRS, 3L);
  curl_easy_setopt(punho.get(), CURLOPT_CONNECTTIMEOUT, 4L);
  curl_easy_setopt(punho.get(), CURLOPT_TIMEOUT, 8L);
  curl_easy_setopt(punho.get(), CURLOPT_NOSIGNAL, 1L);
  curl_easy_setopt(punho.get(), CURLOPT_NOPROGRESS, 0L);
  curl_easy_setopt(punho.get(), CURLOPT_XFERINFOFUNCTION, consulta_interrupcao);
  const auto desfecho = curl_easy_perform(punho.get());
  anterior = std::chrono::steady_clock::now();
  if (desfecho != CURLE_OK)
    return recusa("LRCLIB: " + std::string(curl_easy_strerror(desfecho)));
  long estado = 0;
  curl_easy_getinfo(punho.get(), CURLINFO_RESPONSE_CODE, &estado);
  if (estado == 429) {
    long long espera = 60;
    if (!espera_pedida.empty()) {
      char* fim = nullptr;
      errno = 0;
      const auto segundos = std::strtoll(espera_pedida.c_str(), &fim, 10);
      if (errno == 0 && fim == espera_pedida.c_str() + espera_pedida.size() && segundos >= 0)
        espera = segundos;
      else {
        const auto data = curl_getdate(espera_pedida.c_str(), nullptr);
        if (data >= 0) espera = std::max<long long>(0, data - std::time(nullptr));
      }
    }
    bloqueado_ate = espera > 31536000 ? std::chrono::steady_clock::time_point::max()
        : anterior + std::chrono::seconds(espera);
  }
  return interpreta_busca_da_letra(estado, corpo, artista, titulo, duracao, letra, razao);
}

namespace {

// carimbo, lê `[mm:ss.cc]` ou `[mm:ss]` no principio do que resta. Devolve falso
// quando não ha carimbo alli, e ahi `cursor` não se mexe.
bool carimbo(std::string_view linha, std::size_t* cursor, double* tempo) {
  std::size_t i = *cursor;
  if (i >= linha.size() || linha[i] != '[') return false;
  ++i;
  const std::size_t principio_min = i;
  while (i < linha.size() && std::isdigit(static_cast<unsigned char>(linha[i]))) ++i;
  if (i == principio_min || i >= linha.size() || linha[i] != ':') return false;
  const int minutos = std::atoi(std::string(linha.substr(principio_min, i - principio_min)).c_str());
  ++i;
  const std::size_t principio_seg = i;
  while (i < linha.size() &&
         (std::isdigit(static_cast<unsigned char>(linha[i])) || linha[i] == '.' ||
          linha[i] == ':'))
    ++i;
  if (i == principio_seg || i >= linha.size() || linha[i] != ']') return false;
  // O separador dos centesimos é ponto ou DOUS PONTOS: o fórmato admitte os dous,
  // e ha gerador que usa o segundo. Troca-se antes de converter.
  std::string segundos(linha.substr(principio_seg, i - principio_seg));
  for (char& letra : segundos) if (letra == ':') letra = '.';
  *tempo = minutos * 60.0 + std::atof(segundos.c_str());
  *cursor = i + 1;
  return true;
}

}  // namespace

std::vector<LinhaDaLetra> analysa_lrc(std::string_view texto) {
  std::vector<LinhaDaLetra> linhas;
  std::size_t principio = 0;
  while (principio <= texto.size()) {
    std::size_t fim = texto.find('\n', principio);
    if (fim == std::string_view::npos) fim = texto.size();
    std::string_view linha = texto.substr(principio, fim - principio);
    if (!linha.empty() && linha.back() == '\r') linha.remove_suffix(1);
    principio = fim + 1;

    // Todos os carimbos da frente, e sómente depois o texto: o mesmo verso pode
    // trazer varios tempos, e cada um d'elles gera a sua linha.
    std::vector<double> tempos;
    std::size_t cursor = 0;
    double tempo = 0.0;
    while (carimbo(linha, &cursor, &tempo)) tempos.push_back(tempo);
    if (tempos.empty()) continue;  // linha sem carimbo: cabeçalho ou lixo

    std::string corpo(linha.substr(cursor));
    // Apara-se sómente o espaço da FRENTE: o do fim pode ser intencional em letra
    // que se alinhe, e tirá-lo não melhora cousa alguma.
    std::size_t desde = 0;
    while (desde < corpo.size() &&
           std::isspace(static_cast<unsigned char>(corpo[desde])) != 0)
      ++desde;
    corpo.erase(0, desde);
    for (const double quando : tempos) linhas.push_back({quando, corpo});
    if (fim == texto.size()) break;
  }
  std::stable_sort(linhas.begin(), linhas.end(),
                   [](const LinhaDaLetra& a, const LinhaDaLetra& b) {
                     return a.tempo < b.tempo;
                   });
  return linhas;
}

int linha_corrente(const std::vector<LinhaDaLetra>& linhas, double posicao) {
  if (linhas.empty()) return -1;
  if (!std::isfinite(posicao) || posicao < linhas.front().tempo) return -1;
  // Busca binaria: a ultima linha cujo tempo não passa da posição. Linear serviria
  // n'uma letra de cincoenta versos, mas isto corre vinte vezes por segundo, e
  // n'uma letra longa somma.
  std::size_t baixo = 0, alto = linhas.size();
  while (baixo + 1 < alto) {
    const std::size_t meio = baixo + (alto - baixo) / 2;
    if (linhas[meio].tempo <= posicao) baixo = meio;
    else alto = meio;
  }
  return static_cast<int>(baixo);
}

std::vector<LinhaDaLetra> le_lrc_do_disco(const std::filesystem::path& audio) {
  const std::filesystem::path onde = caminho_do_lrc(audio);
  std::ifstream fonte(onde, std::ios::binary);
  if (!fonte) return {};  // não ha letra: caso ordinario, e não erro
  const std::string texto((std::istreambuf_iterator<char>(fonte)),
                          std::istreambuf_iterator<char>());
  return analysa_lrc(texto);
}

std::vector<std::string> le_letra_plana_do_disco(const std::filesystem::path& audio) {
  auto caminho = caminho_do_lrc(audio);
  caminho.replace_extension(".letra.txt");
  std::ifstream arquivo(caminho, std::ios::binary);
  std::vector<std::string> versos;
  std::string verso;
  std::size_t lidos = 0;
  while (std::getline(arquivo, verso) && lidos < 1024 * 1024) {
    lidos += verso.size();
    if (!verso.empty() && verso.back() == '\r') verso.pop_back();
    versos.push_back(std::move(verso));
  }
  return versos;
}

}  // namespace mysong::nucleo

//   Da lavra do eminente Doutor BRAGA US., Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
