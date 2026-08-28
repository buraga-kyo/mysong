// ══════════════════════════════════════════════════════════════════════════
//   TRACTADO DOS AJUSTES — src/nucleo/ajustes.cpp
// ══════════════════════════════════════════════════════════════════════════
// A obra dos ajustes, em partes que se não misturam: o LEITOR, que parte o
// texto em pares e nada sabe do mundo; o RESOLVEDOR, que escolhe entre os
// quatro degraus e nada abre; e as funcções que tocam disco e ambiente.
//
// INVARIANTE ...... arquivo algum se abre para escripta n'esta unidade.
// Q.E.D. .......... puros o leitor e o resolvedor, a bateria prova o formato e
//                   a precedencia inteira sem tocar disco nem ambiente.
// ══════════════════════════════════════════════════════════════════════════
#include "nucleo/ajustes.hpp"

#include <charconv>
#include <string>
#include <system_error>
#include <utility>

namespace mysong::nucleo {

// nome_da_origem — a palavra do --sonda. Switch exhaustivo, e não taboa: origem
// nova accende aviso do compilador em vez de sahir calada como «sem nome».
std::string_view nome_da_origem(Origem origem) {
  switch (origem) {
    case Origem::Argumento: return "argumento";
    case Origem::Ambiente: return "ambiente";
    case Origem::Arquivo: return "arquivo";
    case Origem::Padrao: return "padrão";
  }
  return "origem sem nome";
}

// queixa — accrescenta, até o tecto. Alcançado elle, deixa-se UMA linha a dizer
// que ha mais: é o que faz um binario passado como conf caber n'uma tela.
void Ajustes::queixa(std::string dito) {
  if (queixas.size() < QUEIXAS_NO_MAXIMO) {
    queixas.push_back(std::move(dito));
  } else if (queixas.size() == QUEIXAS_NO_MAXIMO) {
    queixas.push_back("e ha mais queixas, que o tecto de " +
                      std::to_string(QUEIXAS_NO_MAXIMO) + " se alcançou");
  }
}

// aparar — tira os brancos das DUAS pontas, e sómente das pontas: branco no
// MEIO do valor é do valor, que caminho com espaço é caminho legitimo.
std::string_view aparar(std::string_view texto) {
  const auto branco = [](char letra) {
    return letra == ' ' || letra == '\t' || letra == '\r' || letra == '\v' ||
           letra == '\f';
  };
  while (!texto.empty() && branco(texto.front())) texto.remove_prefix(1);
  while (!texto.empty() && branco(texto.back())) texto.remove_suffix(1);
  return texto;
}

// corta_commentario — o `#` abre commentario até o fim da linha, em QUALQUER
// ponto, e não ha aspas nem escape que o façam literal. O limite é DECLARADO, e
// não descuido: caminho que traga cerquilha fica inexprimivel, e regra com
// excepção seria regra que o operador não adivinha olhando o proprio arquivo.
std::string_view corta_commentario(std::string_view linha) {
  const std::size_t cerquilha = linha.find('#');
  if (cerquilha == std::string_view::npos) return linha;
  return linha.substr(0, cerquilha);
}

// ler_pares — o LEITOR do formato: linhas «chave = valor», com o `#` a abrir
// commentario, os brancos aparados nas pontas, e a linha vazia ignorada. A
// chave repetida NÃO se resolve aqui: os pares sahem na ordem em que vieram, e
// quem os consome adeante toma o ultimo, que é o costume de todo arquivo de
// linhas e o que deixa o operador corrigir accrescentando no fim.
//
// Nada aqui é fatal. Linha que se não entende vira QUEIXA e segue: arquivo
// velho, escripto para uma versão anterior d'esta obra, não ha de impedir a
// obra de abrir, que é o que a issue #67 manda por extenso.
std::vector<Par> ler_pares(std::string_view texto, Ajustes* ajustes) {
  std::vector<Par> pares;
  std::size_t numero = 0;
  while (!texto.empty()) {
    const std::size_t quebra = texto.find('\n');
    const bool ultima = quebra == std::string_view::npos;
    const std::string_view crua = texto.substr(0, ultima ? texto.size() : quebra);
    texto.remove_prefix(crua.size() + (ultima ? 0 : 1));
    const std::string onde = "linha " + std::to_string(++numero) + ": ";
    if (crua.size() > LINHA_NO_MAXIMO) {
      ajustes->queixa(onde + "comprida de mais; ignorada");
      continue;
    }
    const std::string_view linha = aparar(corta_commentario(crua));
    if (linha.empty()) continue;
    const std::size_t egual = linha.find('=');
    if (egual == std::string_view::npos) {
      ajustes->queixa(onde + "sem o signal de egual; ignorada");
      continue;
    }
    const std::string_view chave = aparar(linha.substr(0, egual));
    if (chave.empty()) {
      ajustes->queixa(onde + "sem chave antes do egual; ignorada");
      continue;
    }
    for (const Par& antigo : pares)
      if (antigo.chave == chave)
        ajustes->queixa(onde + "a chave «" + std::string(chave) +
                        "» já veio na linha " + std::to_string(antigo.linha) +
                        "; vale a ultima");
    pares.push_back({numero, std::string(chave),
                     std::string(aparar(linha.substr(egual + 1)))});
  }
  return pares;
}

namespace {

// egual_sem_caixa — compara cego á caixa, em ASCII, e faz a caixa á mão pela
// razão que o contem_insensivel da sonda já tem escripta: sob locale turco o
// 'I' desce a caractere que não é 'i', e «YouTube» fugiria da comparação por
// motivo que ninguem havia de suspeitar.
bool egual_sem_caixa(std::string_view esta, std::string_view aquella) {
  const auto baixa = [](char letra) {
    return letra >= 'A' && letra <= 'Z' ? static_cast<char>(letra | 0x20)
                                        : letra;
  };
  if (esta.size() != aquella.size()) return false;
  for (std::size_t passo = 0; passo < esta.size(); ++passo)
    if (baixa(esta[passo]) != baixa(aquella[passo])) return false;
  return true;
}

// inteiro_de — o numero INTEIRAMENTE consumido, e nunca o prefixo d'elle: «70
// lixo» não é setenta. Vae por from_chars, e não por stoi: aquelle accusa o
// estouro em vez de o dobrar, e não lança; excepção pela borda d'este modulo
// derrubaria a obra por causa de um erro de dedo no arquivo do operador.
std::optional<int> inteiro_de(std::string_view texto) {
  if (!texto.empty() && texto.front() == '+') texto.remove_prefix(1);
  int valor = 0;
  const char* const fim = texto.data() + texto.size();
  const std::from_chars_result colhido =
      std::from_chars(texto.data(), fim, valor);
  if (colhido.ec != std::errc() || colhido.ptr != fim) return std::nullopt;
  return valor;
}

}  // namespace

// fonte_de — as tres da issue #56, e sómente ellas. Nome que não é nenhuma
// d'ellas devolve vazio, e não a primeira da lista: acceitar por approximação
// faria o operador buscar no Spotify a pensar que buscava no YouTube.
std::optional<Fonte> fonte_de(std::string_view texto) {
  if (egual_sem_caixa(texto, "youtube")) return Fonte::YouTube;
  if (egual_sem_caixa(texto, "youtube-music")) return Fonte::YouTubeMusic;
  if (egual_sem_caixa(texto, "spotify")) return Fonte::Spotify;
  return std::nullopt;
}

std::optional<int> volume_de(std::string_view texto) {
  const std::optional<int> numero = inteiro_de(texto);
  if (!numero || *numero < 0 || *numero > VOLUME_DA_CASA) return std::nullopt;
  return numero;
}

// baixas_de — de uma até o tecto, e zero recusa-se: obreiro nenhum é fila que
// nunca anda, e o operador ficaria a ver a baixa «na espera» para sempre sem
// entender por que. O tecto está no cabeçalho, com a razão d'elle.
std::optional<std::size_t> baixas_de(std::string_view texto) {
  const std::optional<int> numero = inteiro_de(texto);
  if (!numero || *numero < 1 || *numero > BAIXAS_NO_MAXIMO) return std::nullopt;
  return static_cast<std::size_t>(*numero);
}

// resolver — a ESCADA, assentada de baixo para cima: o padrão primeiro, e o
// arquivo a escrever por cima. Percorrer os pares na ORDEM em que vieram é o
// que faz a chave repetida valer a ultima, sem regra propria para isso: a
// segunda occorrencia sobrescreve a primeira, e é tudo.
void resolver(const Degraus& degraus,
              const std::filesystem::path& padrao_do_acervo,
              const Aferidor& ha_directorio, Ajustes* ajustes) {
  ajustes->acervo = {padrao_do_acervo, Origem::Padrao};
  for (const Par& par : degraus.arquivo) {
    const std::string onde = "linha " + std::to_string(par.linha) + ": ";
    if (par.chave == "acervo") {
      if (ha_directorio(par.valor))
        ajustes->acervo = {std::filesystem::path(par.valor), Origem::Arquivo};
      else
        ajustes->queixa(onde + "acervo «" + par.valor +
                       "» não é directorio que exista; vale o de baixo");
    } else if (par.chave == "volume") {
      if (const auto numero = volume_de(par.valor))
        ajustes->volume = {*numero, Origem::Arquivo};
      else
        ajustes->queixa(onde + "volume «" + par.valor +
                       "» não é numero de zero a cem; vale o degrau de baixo");
    } else if (par.chave == "fonte_da_busca") {
      if (const auto fonte = fonte_de(par.valor))
        ajustes->fonte_da_busca = {*fonte, Origem::Arquivo};
      else
        ajustes->queixa(onde + "fonte_da_busca «" + par.valor +
                       "» não é youtube, youtube-music nem spotify");
    } else if (par.chave == "baixas_simultaneas") {
      if (const auto quantas = baixas_de(par.valor))
        ajustes->baixas_simultaneas = {*quantas, Origem::Arquivo};
      else
        ajustes->queixa(onde + "baixas_simultaneas «" + par.valor +
                       "» não é numero de um a oito");
    } else {
      ajustes->queixa(onde + "chave desconhecida «" + par.chave + "»; ignorada");
    }
  }
  // O AMBIENTE, e vae CRÚ: quem poz a variavel no perfil do shell manda, e o
  // programa não julga o caminho d'ella. Aferil-a mudaria o acervo debaixo dos
  // pés de quem aponta para monte de rede que ainda não montou, e a variavel
  // existe justamente para esse caso.
  if (degraus.acervo_do_ambiente)
    ajustes->acervo = {std::filesystem::path(*degraus.acervo_do_ambiente),
                      Origem::Ambiente};
  // O ARGUMENTO, que é o degrau de cima, e este AFERE-SE: quem o digita está a
  // olhar para a tela agora, e ha de saber já que errou o caminho.
  if (degraus.acervo_do_argumento) {
    if (ha_directorio(*degraus.acervo_do_argumento))
      ajustes->acervo = {std::filesystem::path(*degraus.acervo_do_argumento),
                        Origem::Argumento};
    else
      ajustes->queixa("--acervo «" + *degraus.acervo_do_argumento +
                     "» não é directorio que exista; vale o de baixo");
  }
}

}  // namespace mysong::nucleo

//   Da lavra do eminente Doutor BRAGA US. — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
