// ══════════════════════════════════════════════════════════════════════════
//   TRACTADO DA AQUISIÇÃO — src/nucleo/aquisicao.hpp
// ══════════════════════════════════════════════════════════════════════════
// A UNICA camada d'esta obra que toca a rede. Chama o `yt-dlp`, guarda o audio na
// hierarchia do acervo, e escreve as etiquetas ELLA MESMA, com a taglib.
//
// Porque a etiqueta se escreve aqui, e não se deixa ao yt-dlp: medido na issue
// #34 sobre um acervo de verdade, o `yt-dlp --embed-metadata` grava `artist` com
// o nome do CANAL e `title` com o titulo do video inteiro. Quem baixa Bach de um
// canal chamado «Public Domain Classical Music» fica com esse por artista, e o
// acervo passa a estar organizado por canal de YouTube. Esta Casa não aceita
// isso: o operador diz o artista, e o que se grava é o que elle disse.
//
// DOMÍNIO ......... uma URL, e o que o operador quiser dizer sobre a faixa.
// CONTRA-DOMÍNIO .. um arquivo de audio no logar certo, com as etiquetas certas.
// INVARIANTE ...... nome algum vindo da rede chega ao systema de arquivos sem
//                   passar pelo saneamento: barra, NUL e ponto inicial saem.
//                   Baixar NUNCA sobrescreve arquivo que já exista.
// Q.E.D. .......... sendo puras a construcção do caminho, o saneamento e a lista
//                   de argumentos, a bateria afere o que se HA DE correr sem
//                   correr cousa alguma, e sem tocar a rede.
// ══════════════════════════════════════════════════════════════════════════
#pragma once

#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace mysong::nucleo {

// O que se sabe de uma faixa que se vae baixar. Campo vazio quer dizer «não sei»,
// e ahi vale o que a sonda da URL tiver dito.
struct Pedido {
  std::string url;
  std::string artista;
  std::string album;
  std::string titulo;
  int numero = 0;
};

// saneia_nome — o nome que se ha de pôr no systema de arquivos. Tira a barra, o
// NUL e o ponto inicial; apara os espaços das pontas; e corta o comprimento, que
// o ext4 não aceita nome de mais de duzentos e cincoenta e cinco octetos. Nome
// que se reduza a nada devolve «sem titulo», que é resposta e não erro: arquivo
// sem nome não se pode gravar.
std::string saneia_nome(std::string_view crua);

// O DESFECHO de uma aquisição. Toda falha tem nome, porque «falhou» não diz ao
// operador se ha de tentar outra vez, corrigir a URL ou installar o yt-dlp.
enum class Colheita {
  Colhido,          // o arquivo está no logar, com as etiquetas
  SemFerramenta,    // o yt-dlp não está no caminho
  UrlRecusada,      // o yt-dlp não conseguiu ler a URL
  JaExiste,         // ha arquivo no destino; NADA se tocou
  FalhouAoBaixar,   // o yt-dlp sahiu com erro
  FalhouAEtiqueta,  // baixou-se, mas a etiqueta não se pôde escrever
};

// A ETIQUETA que a sonda da URL colheu. Campo vazio quer dizer que a rede não o
// soube dizer, e não que elle seja vazio.
struct EtiquetaRemota {
  std::string titulo;
  std::string canal;
  std::string artista;
  std::string album;
  int numero = 0;
  int duracao = 0;
};

// resolve — junta o que o operador disse com o que a rede disse, e o operador
// GANHA sempre. O artista, faltando os dous, fica «Desconhecido»; e faltando o
// operador mas havendo canal, é o CANAL que se usa, com a ressalva de que se
// registra que se deduziu.
Pedido resolve(const Pedido& pedido, const EtiquetaRemota& remota);

// destino — o caminho na hierarchia do acervo: `<raiz>/Artista/Álbum/NN - Titulo`.
// Sem numero, sahe `Artista/Álbum/Titulo`; sem album, `Artista/Titulo`. A extensão
// NÃO entra: quem a põe é o yt-dlp, que é quem sabe em que fórma sahiu o audio.
std::filesystem::path destino(const std::filesystem::path& raiz,
                              const Pedido& pedido);

// argumentos_da_sonda — o que se corre para PERGUNTAR pela URL, sem baixar. Seis
// campos por `--print`, um por linha, na ordem em que le_etiqueta_remota os lê.
std::vector<std::string> argumentos_da_sonda(const std::string& url);

// argumentos_do_download — o que se corre para BAIXAR. `--no-overwrites` está lá
// de proposito, e é a segunda guarda: a primeira é a checagem do destino, e ter
// as duas quer dizer que uma corrida entre duas aquisições não perde arquivo.
std::vector<std::string> argumentos_do_download(
    const std::string& url, const std::filesystem::path& molde);

// le_etiqueta_remota — as seis linhas que a sonda imprimiu. Linha «NA» ou vazia é
// campo que a rede não soube dizer.
EtiquetaRemota le_etiqueta_remota(const std::string& sahida);

// ── E AGORA O QUE TOCA O MUNDO. Estas tres não são puras, e é de proposito que
// elas vivem juntas no fim: o que se prova está acima, o que se não prova está
// aqui, e o olho vê a fronteira de um relance.

// corre — corre o commando e colhe a sahida. Não ha shell: `execvp` recebe o
// vector tal e qual. Devolve o codigo de sahida, e menos um se nem se pôde
// erguer o processo.
int corre(const std::vector<std::string>& argumentos, std::string* colhido);

// sonda_url — pergunta á rede o que ella sabe da URL. Falso quando o yt-dlp não
// respondeu; ahi a etiqueta fica como estava.
bool sonda_url(const std::string& url, EtiquetaRemota* remota);

// baixa — o acto inteiro: resolve, monta o caminho, cria o directorio, chama o
// yt-dlp, e escreve a etiqueta com a taglib. `gravado` recebe o caminho do
// arquivo que ficou, quando ficou algum.
Colheita baixa(const std::filesystem::path& raiz, const Pedido& pedido,
               std::filesystem::path* gravado);

}  // namespace mysong::nucleo

//   Da lavra do eminente Doutor BRAGA US. — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
