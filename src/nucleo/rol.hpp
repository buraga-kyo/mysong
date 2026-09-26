// ══════════════════════════════════════════════════════════════════════════
//   TRACTADO DO ROL, src/nucleo/rol.hpp
// ══════════════════════════════════════════════════════════════════════════
// As LISTAS do operador: o que elle juntou á mão, na ordem em que quiz. Vivem
// n'um banco PROPRIO, e não no da bibliotheca, e a razão não é gosto.
//
// A bibliotheca é RECONSTRUIDA a cada varredura: o Escriba lavra n'um temporario
// e renomeia por cima do antigo. Taboa de lista no mesmo arquivo seria apagada em
// toda varredura, e o operador perderia a lista por ter mandado varrer o disco.
// Donde arquivo á parte, `rol.sqlite3` ao lado do índice.
//
// A atomicidade tambem é differente, e por bom motivo. A bibliotheca escreve-se
// de uma vez e por isso o temporario e o rename servem-lhe. A lista muta-se linha
// a linha, e reconstruir o arquivo a cada linha perderia a lista da mão alheia
// que estivesse a mutar outra; aqui a atomicidade é a da TRANSACÇÃO, com
// journal_mode=DELETE e synchronous=FULL, que é a mesma garantia por operação.
//
// DOMÍNIO ......... um caminho de banco, que ENTRA POR PARÂMETRO, e os nomes e
//                   caminhos que o operador disse.
// CONTRA-DOMÍNIO .. as listas, e as faixas de cada uma na ordem gravada.
// INVARIANTE ...... a ordem é EXPLICITA e CONTIGUA: os itens de uma lista têm
//                   ordem 0, 1, 2, ... sem buraco, e retirar um fecha o buraco.
//                   Sem isso, mover para cima teria de adivinhar quem é o vizinho.
// Q.E.D. .......... entrando o caminho por parâmetro, a bateria corre inteira em
//                   directorio temporario, e prova alguma pode tocar as listas de
//                   quem nos usa.
// ══════════════════════════════════════════════════════════════════════════
#pragma once

#include <filesystem>
#include <functional>
#include <string>
#include <string_view>
#include <vector>

struct sqlite3;

namespace mysong::nucleo {

// A VERSÃO do esquema das listas. Independente da da bibliotheca: são bancos
// differentes, e fazê-las subir juntas obrigaria a reconstruir um por causa do outro.
inline constexpr int kVersaoDoRol = 1;

// Uma LISTA. `quantos` vem da conta, e não de columna guardada: columna guardada
// é numero que se pode desencontrar do que ha, e desencontrar-se-hia no dia em que
// alguem apagasse item sem a decrementar.
struct Rol {
  int id = 0;
  std::string nome;
  int quantos = 0;
};

// saneia_nome_de_rol, apara os brancos das pontas e corta o comprimento. Nome
// que se reduza a nada devolve cadeia VAZIA, e cadeia vazia não se aceita: lista
// sem nome não se pode eleger na tela.
std::string saneia_nome_de_rol(std::string_view crua);

// O ROLEIRO. Abre para LER E ESCREVER, e cria o banco não havendo nenhum: a
// primeira lista do operador não ha de falhar por falta de arquivo.
class Roleiro {
 public:
  explicit Roleiro(std::filesystem::path banco);
  ~Roleiro();

  Roleiro(const Roleiro&) = delete;
  Roleiro& operator=(const Roleiro&) = delete;

  bool aberto() const noexcept;
  int versao() const noexcept;

  std::vector<Rol> rois() const;

  // cria, o id da lista nova, e ZERO quando o nome é vazio depois de saneado ou
  // quando já existe lista com esse nome. Nome repetido não se aceita: duas listas
  // do mesmo nome na tela não se distinguem.
  int cria(std::string_view nome);

  bool renomeia(int id, std::string_view nome);

  // apaga, a lista e os itens d'ella. A confirmação NÃO é d'aqui: quem pergunta
  // ao operador é a tela, e esta camada obedece.
  bool apaga(int id);

  // junta, a faixa no FIM da lista. Repetida não se recusa: quem quer a mesma
  // faixa duas vezes na mesma lista tem direito a ella.
  bool junta(int id, std::string_view caminho);

  // retira, o item de ordem `ordem`, e FECHA o buraco que elle deixou.
  bool retira(int id, int ordem);

  // retira_de_todos, o caminho sae de TODAS as listas, e cada uma fecha os seus
  // buracos. Devolve quantas occorrencias sahiram. É o que a faixa mandada á
  // lixeira pede: deixá-la nas listas faria a lista apontar para arquivo que já
  // não está no disco, e tocá-la daria silencio sem se saber porquê.
  int retira_de_todos(std::string_view caminho);

  // troca, os itens de duas ordens vizinhas ou não. Falso quando alguma das duas
  // não existe. É por aqui que sobe e desce se cumprem.
  bool troca(int id, int uma, int outra);

  // faixas, os caminhos na ordem gravada.
  std::vector<std::string> faixas(int id) const;

 private:
  std::filesystem::path banco_;
  sqlite3* punho_ = nullptr;
};

}  // namespace mysong::nucleo

//   Da lavra do eminente Doutor BRAGA US., Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
