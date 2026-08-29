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
// A MINIATURA, porem, aceita-se, e a distincção não é capricho (issue #81). O que
// se recusa ao yt-dlp é o TEXTO da etiqueta, que elle inventa mal; a arte da capa
// elle a colhe da propria pagina, e n'isso não ha o que inventar. Pedi-la é o unico
// modo de a Casa ter capa alguma: sem ella, nada de arte se grava, nem em arquivo
// nem em quadro APIC, e o painel NOW PLAYING fica com o marcador para sempre.
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

#include "nucleo/catalogo.hpp"
#include "nucleo/musicbrainz.hpp"

namespace mysong::nucleo {

// A FONTE da busca (issue #56). As tres CONVIVEM, e nenhuma substitue outra: o
// operador escolhe de onde a busca vem, e a eleita acompanha o Pedido até a fila
// de baixa, para que quem baixa não precise saber quem pediu.
enum class Fonte { YouTube, YouTubeMusic, Spotify };

// nome_da_fonte — a palavra que o cabeçalho da busca mostra. Vive aqui, e não na
// tela, pela mesma razão de razao_da_colheita: fonte nova sem nome não compila.
std::string_view nome_da_fonte(Fonte fonte);

// proxima_fonte — o ciclo fechado da tecla que troca: YouTube, YouTube Music,
// Spotify, e volta ao começo. A ordem prova-se, em vez de morar na tela.
Fonte proxima_fonte(Fonte fonte);

// O que se sabe de uma faixa que se vae baixar. Campo vazio quer dizer «não sei»,
// e ahi vale o que a sonda da URL tiver dito.
struct Pedido {
  std::string url;
  std::string artista;
  std::string album;
  std::string titulo;
  int numero = 0;
  // A FONTE que pediu (issue #56). Um pedido sem URL busca o audio na fonte
  // D'ELLE, e não n'uma que a fila de baixa tivesse de adivinhar.
  Fonte fonte = Fonte::YouTube;
  // A DURAÇÃO esperada, em segundos, e zero é «não sei». Entra na issue #13: com
  // ella, e sómente com ella, a URL pode vir VAZIA e a baixa busca o audio por si,
  // casando o achado pela duração. Sem ella não ha casamento de que se possa
  // confiar, e a faixa sahe por duvidosa em vez de baixar cousa errada calada.
  int duracao = 0;
  // O ID do track no Spotify (issue #57), vindo do catalogo: é por elle que o
  // MusicBrainz acha a GRAVAÇÃO exacta, e não uma parecida. Vazio quando a
  // faixa não veio do catalogo, e ahi a resolução tenta a busca por titulo.
  std::string id_spotify;
  // O ANO do lançamento, e zero é «não sei»; havendo-o, elle vae á etiqueta. Dão-no
  // DUAS fontes: a busca da issue #56 (a sonda da URL não o pergunta) e a release
  // canonica que o MusicBrainz da issue #57 elege, que ganha quando a gravação casa.
  int ano = 0;
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
  // Baixou-se pelo criterio de hoje (titulo e duração), SEM a gravação casada
  // (issue #57, RULINGS R2): o arquivo ficou no disco, mas o casamento aceita
  // cover e versão ao vivo, e dizer Colhido seria confiança que elle não tem.
  ColhidoDuvidoso,
  SemFerramenta,    // o yt-dlp não está no caminho
  UrlRecusada,      // o yt-dlp não conseguiu ler a URL
  JaExiste,         // ha arquivo no destino; NADA se tocou
  FalhouAoBaixar,   // o yt-dlp sahiu com erro
  FalhouAEtiqueta,  // baixou-se, mas a etiqueta não se pôde escrever
  Duvidosa,         // achado algum casou com confiança; NADA se baixou
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

// enriquece — o pedido com o album, o anno e o numero CANONICOS da ficha do
// MusicBrainz por cima dos de hoje, que eram o nome da lista por album e a
// posição na lista por numero (issue #57). Artista e titulo não se tocam: são o
// que o operador vê e o que a busca usa. Campo que a ficha não diga fica como
// estava, e ficha vazia devolve o pedido tal e qual: é o caminho de quando o
// MusicBrainz não casou.
Pedido enriquece(const Pedido& pedido, const FichaMB& ficha);

// destino — o caminho na hierarchia do acervo: `<raiz>/Artista/Álbum/NN - Titulo`.
// Sem numero, sahe `Artista/Álbum/Titulo`; sem album, `Artista/Titulo`. A extensão
// NÃO entra: quem a põe é o yt-dlp, que é quem sabe em que fórma sahiu o audio.
std::filesystem::path destino(const std::filesystem::path& raiz,
                              const Pedido& pedido);

// ── AS BANDEIRAS DO MOTOR (issue #55) ───────────────────────────────────────

// O YouTube passou a exigir que o cliente resolva um desafio em JavaScript antes
// de entregar o audio. Sem motor, o yt-dlp avisa que a assinatura falhou e não
// colhe fórmato algum. O motor que esta maquina tem é o `node`, e o yt-dlp habilita
// apenas o `deno` por conta propria, de sorte que dizer-lhe qual é não é ornamento.
inline constexpr char kMotorDeJs[] = "node";

// A biblioteca solucionadora não vem no yt-dlp: elle a busca quando se lhe permitte.
inline constexpr char kComponenteDoDesafio[] = "ejs:github";

// O navegador de onde o cookie sahe, QUANDO se pedir. MEDIDO em 2026-08-26: com o
// cookie do chrome o yt-dlp responde «The page needs to be reloaded.» a toda URL, e
// SEM elle colhe. Por isso o cookie é opção desligada, e não remedio de omissão:
// serve ao video restricto por idade, e a nada mais.
inline constexpr char kNavegadorDoCookie[] = "chrome";

// bandeiras_do_motor — as bandeiras que TODA chamada ao yt-dlp carrega. Vivem n'uma
// função só para que sejam UM logar: quem as tirar d'aqui derruba as tres provas de
// uma vez, e não uma só, que é como se sabe que a guarda está viva.
std::vector<std::string> bandeiras_do_motor(bool com_cookie);

// argumentos_da_sonda — o que se corre para PERGUNTAR pela URL, sem baixar. Seis
// campos por `--print`, um por linha, na ordem em que le_etiqueta_remota os lê.
std::vector<std::string> argumentos_da_sonda(const std::string& url,
                                             bool com_cookie = false);

// argumentos_do_download — o que se corre para BAIXAR. `--no-overwrites` está lá
// de proposito, e é a segunda guarda: a primeira é a checagem do destino, e ter
// as duas quer dizer que uma corrida entre duas aquisições não perde arquivo.
// Traz tambem a miniatura, convertida e embutida; a razão está no corpo.
std::vector<std::string> argumentos_do_download(
    const std::string& url, const std::filesystem::path& molde,
    bool com_cookie = false);

// le_etiqueta_remota — as seis linhas que a sonda imprimiu. Linha «NA» ou vazia é
// campo que a rede não soube dizer.
EtiquetaRemota le_etiqueta_remota(const std::string& sahida);

// razao_da_colheita — a palavra que se mostra ao operador. Vive aqui, e não na
// tela, para que quem acrescente um desfecho seja obrigado a nomeá-lo: o `switch`
// é exhaustivo, e desfecho novo sem palavra não compila.
std::string_view razao_da_colheita(Colheita colheita);

// ── O CASAMENTO PELO CATALOGO (issue #13) ───────────────────────────────────

// A TOLERANCIA do casamento, em segundos. DERIVA de kToleranciaSeg, que mora em
// musicbrainz.hpp e é a fonte do numero (issue #63); o nome fica, que é por elle
// que a Casa a chama desde a issue #13. Quem quiser mudar os doze muda a fonte,
// e não esta linha, que d'antes declarava o mesmo numero uma segunda vez.
inline constexpr int TOLERANCIA_DO_CASAMENTO = kToleranciaSeg;

// ── A BUSCA NO YOUTUBE (issue #12) ──────────────────────────────────────────

// Um ACHADO da busca. É o que a tela mostra, e o que a baixa consome.
struct Achado {
  std::string titulo;
  std::string canal;
  int duracao = 0;  // em segundos; zero é «não disse»
  std::string url;
  // Os campos da MUSICA (issue #56): nas art tracks do YouTube Music vêm
  // preenchidos, e na busca comum vêm NA, que a leitura torna vazio. A `faixa` é
  // o titulo CANONICO (%(track)s), limpo do que o titulo do video carregue.
  std::string artista;
  std::string album;
  std::string faixa;
  int ano = 0;
  // A posição na lista de origem, quando a fonte a tem (o catalogo do Spotify
  // tem; a busca na rede não). É ella que vira o «NN - » do nome do arquivo.
  int numero = 0;
  // A FONTE de que o achado veio, para que a encommenda a carregue adiante.
  Fonte fonte = Fonte::YouTube;
  // O ID do track no Spotify (issue #57), quando o achado sahiu do catalogo: sem
  // elle a encommenda perderia o casamento pelo LINK e cahiria na busca por
  // titulo dentro do MusicBrainz, que é o caminho segundo e não o primeiro.
  std::string id_spotify;
};

// codifica_para_url — o termo como pedaço de URL: todo byte fóra de
// [A-Za-z0-9._~-] sahe por cento e dous hexadecimaes. O `#` sobretudo: cru, elle
// cortaria a consulta ao meio, que o fragmento da URL da musica é nosso.
std::string codifica_para_url(std::string_view crua);

// O TECTO da fonte da MUSICA. Dez, e não os vinte da comum: sem o flat cada
// achado é uma sonda de rede (~1,3 s medidos), e dez respondem em ~17 s.
inline constexpr int ACHADOS_DA_MUSICA = 10;

// argumentos_da_busca — o que se corre, POR FONTE. YouTube: `ytsearchN:` com
// `--flat-playlist` (abrir cada resultado custaria N sondas). YouTubeMusic: a URL
// de busca do music.youtube.com com o fragmento #songs (só a prateleira das
// musicas; sem elle um album no meio expande a lista inteira d'elle) e SEM flat,
// que com flat os campos da musica vêm NA (medido em 2026-08-27). Spotify: mapa
// para o YouTube, que o audio do catalogo vem de lá (fronteira da issue #13).
std::vector<std::string> argumentos_da_busca(const std::string& termo,
                                             int quantos, Fonte fonte,
                                             bool com_cookie = false);

// le_achados — as linhas que a busca imprimiu, OITO por achado e nessa ordem. Lê-se
// por linha, e não por separador dentro da linha: titulo de video tras barra vertical,
// tabulação e tudo o mais, e um separador seria enganado pelo primeiro d'elles.
std::vector<Achado> le_achados(const std::string& sahida);

// melhor_achado — o indice do achado que casa com o pedido, e MENOS UM não casando
// nenhum. As regras, e cada uma com o seu porque:
//
// 1. duração dentro da tolerancia. É o unico crivo que separa a faixa da versão
//    estendida, e sem elle baixar-se-hia mistura de dez minutos por faixa de tres.
// 2. entre as que passam, ganha a que tras o TITULO do pedido no titulo d'ella.
//    Não passando nenhuma esse segundo crivo, ganha a de duração mais proxima.
// 3. pedido sem duração NÃO casa. Não é descuido: sem duração não ha crivo algum, e
//    a tarefa manda marcar por duvidosa em vez de baixar cousa errada calada.
int melhor_achado(const std::vector<Achado>& achados, const Pedido& pedido,
                  int tolerancia);

// achado_mais_proximo — o indice do achado de duração mais proxima da pedida, e
// menos um sómente quando não ha achado algum. É o eleitor do caminho ISRC
// (issue #57): o termo de busca já nomeia a GRAVAÇÃO, donde o titulo não entra
// e distancia alguma exclue; a duração exacta do MusicBrainz é DESEMPATE, e não
// crivo, por ordem registrada em RULINGS R3. Sem duração pedida, ou sem achado
// que a diga, vale o primeiro achado, que é o que a busca poz á frente.
int achado_mais_proximo(const std::vector<Achado>& achados, int duracao);

// encommenda_do_achado — o Pedido que o achado eleito dá, campo a campo: URL,
// artista, album, titulo CANONICO (a faixa, e não o titulo do video), numero,
// ano e fonte. No achado da busca comum esses campos vêm vazios, donde o pedido
// sahe só com URL e fonte, egual ao de hoje. A DURAÇÃO entra sómente no pedido
// SEM URL: lá ella é o crivo do casamento (issue #13); havendo URL, nada decide.
Pedido encommenda_do_achado(const Achado& achado);

// achados_do_catalogo — a fonte Spotify da tela busca AQUI, no catalogo que a
// issue #13 importou, e rede alguma se toca. O filtro é o da tela: titulo OU
// artista, sem caixa. O album é o nome da LISTA (a pagina de embutir não publica
// album), o numero é a posição, a duração arredonda-se de milesimos, e a URL vae
// vazia de proposito: é o pedido sem URL, o que casa pela duração.
std::vector<Achado> achados_do_catalogo(const Catalogo& catalogo,
                                        const std::string& termo);

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

// busca_na_rede — pergunta ao yt-dlp NA FONTE dita, e estampa-a nos achados: quem
// os consome ha de saber de onde vieram sem perguntar á tela. Vazio quando a rede
// não respondeu, ou quando não ha achado: quem chama distingue-os pelo booleano.
bool busca_na_rede(const std::string& termo, Fonte fonte, int quantos,
                   std::vector<Achado>* achados);

// baixa — o acto inteiro: resolve, monta o caminho, cria o directorio, chama o
// yt-dlp, e escreve a etiqueta com a taglib. `gravado` recebe o caminho do
// arquivo que ficou, quando ficou algum.
Colheita baixa(const std::filesystem::path& raiz, const Pedido& pedido,
               std::filesystem::path* gravado);

}  // namespace mysong::nucleo

//   Da lavra do eminente Doutor BRAGA US. — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
