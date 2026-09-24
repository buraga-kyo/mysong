// ══════════════════════════════════════════════════════════════════════════
//   TRACTADO DO NAVEGADOR, src/tui/navegador.cpp
// ══════════════════════════════════════════════════════════════════════════
// A implementação. Uma regra governa tudo: TODA mudança de vista passa por
// refaz_vista(), e é ella que apara o eleito. Donde não ha caminho por onde o
// eleito saia da lista, e o invariante é estructural e não vigilancia.
//
// DOMÍNIO ......... a bibliotheca, a secção, a trilha e o termo.
// CONTRA-DOMÍNIO .. a vista e o eleito.
// INVARIANTE ...... o eleito está dentro da vista, ou a vista é vazia e elle é
//                   zero. Uma funcção só o garante.
// Q.E.D. .......... havendo uma porta única para a vista, acrescentar secção é
//                   acrescentar um ramo, e não rever a aparadura em N logares.
// ══════════════════════════════════════════════════════════════════════════
#include "tui/navegador.hpp"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <string>
#include <utility>

namespace mysong::tui {

namespace {

// contem_sem_caixa, a comparação do filtro. Sem caixa, que o operador que busca
// «bach» não ha de escrever «Bach» para achar o que já vê na tela. Sem dobra de
// acento, de proposito: dobrar acento em UTF-8 pede taboa que esta Casa não tem,
// e prometter menos é melhor que prometter e falhar no «á» contra o «a».
bool contem_sem_caixa(const std::string& palheiro, const std::string& agulha) {
  if (agulha.empty()) return true;
  const auto baixa = [](unsigned char c) {
    return static_cast<char>(std::tolower(c));
  };
  std::string p, a;
  p.reserve(palheiro.size());
  a.reserve(agulha.size());
  for (const unsigned char c : palheiro) p += baixa(c);
  for (const unsigned char c : agulha) a += baixa(c);
  return p.find(a) != std::string::npos;
}

}  // namespace

// A PORTA UNICA da vista. Toda mudança de secção, de trilha ou de termo passa por
// aqui, e é aqui que o eleito se apara. Não ha segunda aparadura em logar algum:
// havendo duas, uma delas ficaria por corrigir no dia em que a regra mudasse.
void Navegador::refaz_vista() {
  vista_.clear();
  switch (secao_) {
    case Secao::Artistas:
      for (const std::string& nome : livraria_.artistas())
        if (contem_sem_caixa(nome, termo_)) vista_.push_back({nome, nome, 0, 0, {}});
      break;

    case Secao::Albuns:
      for (const std::string& nome : livraria_.albuns(trilha_.front()))
        if (contem_sem_caixa(nome, termo_)) vista_.push_back({nome, nome, 0, 0, {}});
      break;

    case Secao::Faixas:
      for (const nucleo::Faixa& faixa :
           livraria_.faixas_do_album(trilha_.front(), trilha_.back()))
        if (contem_sem_caixa(faixa.titulo, termo_))
          vista_.push_back({faixa.titulo, faixa.caminho, faixa.numero,
                            faixa.duracao, faixa.artista});
      break;

    case Secao::Busca:
      for (const nucleo::Faixa& faixa : livraria_.busca_faixa(termo_))
        vista_.push_back({faixa.titulo, faixa.caminho, faixa.numero,
                          faixa.duracao, faixa.artista});
      break;

    case Secao::Rois:
      // As listas do banco. A conta dos itens vae na columna do numero, que é a que
      // a tabella já sabe pintar: lista de tres faixas mostra tres.
      if (roleiro_ != nullptr)
        for (const nucleo::Rol& rol : roleiro_->rois())
          if (contem_sem_caixa(rol.nome, termo_))
            vista_.push_back(
                {rol.nome, std::to_string(rol.id), rol.quantos, 0, {}});
      break;

    case Secao::NoRol:
      // As faixas de UMA lista, na ordem gravada. O texto é o NOME DO ARQUIVO, e
      // não o titulo da etiqueta: perguntar o titulo de cada uma á bibliotheca seria
      // uma consulta por linha a cada quadro. E a ordem no banco vae na columna do
      // numero, donde retirar e mover não precisam de a adivinhar.
      if (roleiro_ != nullptr) {
        int ordem = 0;
        for (const std::string& caminho : roleiro_->faixas(rol_corrente_)) {
          const std::string curto =
              std::filesystem::path(caminho).filename().string();
          if (contem_sem_caixa(curto, termo_))
            vista_.push_back({curto, caminho, ordem + 1, 0, {}});
          ++ordem;
        }
      }
      break;

    case Secao::Lista:
      // O catalogo lido do Spotify. O INDICE na fonte vae na columna do numero, e é
      // por elle que a faixa eleita se acha: com filtro posto, o indice da vista e o
      // da fonte desencontram-se. É o mesmo engano que a ordem das listas ensinou.
      for (std::size_t i = 0; i < catalogo_.faixas.size(); ++i) {
        const nucleo::FaixaDoCatalogo& qual = catalogo_.faixas[i];
        if (!contem_sem_caixa(qual.titulo, termo_) &&
            !contem_sem_caixa(qual.artista, termo_))
          continue;
        vista_.push_back({qual.titulo, {}, static_cast<int>(i) + 1,
                          qual.duracao_ms / 1000, qual.artista});
      }
      break;

    case Secao::Rede:
      // A UNICA secção que não pergunta á bibliotheca. A fonte é a lista que veio
      // de fóra, e o filtro applica-se sobre ella como sobre as outras.
      //
      // É d'aqui que vem a innocuidade de recarrega() n'esta secção: não havendo
      // consulta ao acervo n'este ramo, a varredura que conclua no meio de o
      // operador escolher um achado refaz a vista IDENTICA, e não lhe apaga a lista.
      // Guarda apartada houve, e sahiu: a mutação que a tirava sobrevivia á bateria.
      // O filtro olha texto OU autor, como na secção Lista: com a fonte de
      // musica o autor é o artista, e buscar por elle é o gesto natural (#56).
      for (const Linha& achado : rede_)
        if (contem_sem_caixa(achado.texto, termo_) ||
            contem_sem_caixa(achado.autor, termo_))
          vista_.push_back(achado);
      break;
  }
  if (vista_.empty()) eleito_ = 0;
  else if (eleito_ >= vista_.size()) eleito_ = vista_.size() - 1;
}

std::size_t primeira_a_mostrar(std::size_t eleito, std::size_t quantas,
                               std::size_t altura,
                               std::size_t primeira_de_antes) {
  if (altura == 0 || quantas == 0) return 0;
  if (quantas <= altura) return 0;  // cabe tudo: não ha rolagem que fazer

  std::size_t primeira = primeira_de_antes;
  // Se a fatia de antes já não cabe na lista, encosta-se ao fim.
  if (primeira + altura > quantas) primeira = quantas - altura;
  // E rola-se o MENOS que baste para o eleito caber.
  if (eleito < primeira) primeira = eleito;
  else if (eleito >= primeira + altura) primeira = eleito - altura + 1;
  return primeira;
}

Navegador::Navegador(const nucleo::Biblioteca& livraria,
                     nucleo::Roleiro* roleiro)
    : livraria_(livraria), roleiro_(roleiro) {
  refaz_vista();
}

Secao Navegador::secao() const noexcept { return secao_; }
const std::vector<Linha>& Navegador::vista() const noexcept { return vista_; }
std::size_t Navegador::eleito() const noexcept { return eleito_; }
const std::vector<std::string>& Navegador::trilha() const noexcept {
  return trilha_;
}
const std::string& Navegador::termo() const noexcept { return termo_; }

void Navegador::desce() noexcept {
  if (vista_.empty()) return;
  if (eleito_ + 1 < vista_.size()) ++eleito_;
}

void Navegador::sobe() noexcept {
  if (eleito_ > 0) --eleito_;
}

void Navegador::ao_principio() noexcept { eleito_ = 0; }

void Navegador::ao_fim() noexcept {
  eleito_ = vista_.empty() ? 0 : vista_.size() - 1;
}

void Navegador::filtra(std::string termo) {
  termo_ = std::move(termo);
  eleito_ = 0;  // termo novo, lista nova: o eleito volta ao alto
  refaz_vista();
}

std::string Navegador::caminho_eleito() const {
  if (vista_.empty()) return {};
  if (secao_ != Secao::Faixas && secao_ != Secao::Busca &&
      secao_ != Secao::NoRol)
    return {};
  return vista_[eleito_].chave;
}

void Navegador::mostra_rois() {
  // O ALVO não se limpa aqui. Limpá-lo faria `a` deixar de funccionar assim que o
  // operador sahisse da lista, e sahir d'ella é justamente o que elle tem de fazer
  // para ir buscar a faixa que quer juntar.
  secao_ = Secao::Rois;
  trilha_.clear();
  termo_.clear();
  eleito_ = 0;
  refaz_vista();
}

int Navegador::rol_corrente() const noexcept { return rol_corrente_; }

const std::string& Navegador::nome_corrente() const noexcept {
  return nome_corrente_;
}

std::vector<nucleo::Rol> Navegador::rois() const {
  if (roleiro_ == nullptr) return {};
  return roleiro_->rois();
}

int Navegador::id_do_eleito() const {
  if (secao_ != Secao::Rois || vista_.empty()) return 0;
  return std::atoi(vista_[eleito_].chave.c_str());
}

std::string Navegador::nome_do_rol_eleito() const {
  if (secao_ == Secao::Rois && !vista_.empty()) return vista_[eleito_].texto;
  if (secao_ == Secao::NoRol && !trilha_.empty()) return trilha_.front();
  return {};
}

bool Navegador::cria_rol(const std::string& nome) {
  if (roleiro_ == nullptr) return false;
  if (roleiro_->cria(nome) == 0) return false;
  // Passa-se á secção das listas, e não se fica onde se estava: quem cria uma
  // lista quer vê-la, e vê-la é o unico modo de conferir que ella nasceu.
  mostra_rois();
  return true;
}

bool Navegador::renomeia_rol(const std::string& nome) {
  const int qual = secao_ == Secao::NoRol ? rol_corrente_ : id_do_eleito();
  if (roleiro_ == nullptr || qual == 0) return false;
  if (!roleiro_->renomeia(qual, nome)) return false;
  const std::string limpo = nucleo::saneia_nome_de_rol(nome);
  if (secao_ == Secao::NoRol && !trilha_.empty()) trilha_.front() = limpo;
  if (qual == rol_corrente_) nome_corrente_ = limpo;
  refaz_vista();
  return true;
}

bool Navegador::apaga_rol() {
  const int qual = secao_ == Secao::NoRol ? rol_corrente_ : id_do_eleito();
  if (roleiro_ == nullptr || qual == 0) return false;
  if (!roleiro_->apaga(qual)) return false;
  // Apagado o ALVO, elle vae-se: apontar para lista que já não existe faria `a`
  // falhar sem dizer porque.
  if (qual == rol_corrente_) {
    rol_corrente_ = 0;
    nome_corrente_.clear();
  }
  // E não ha dentro onde ficar: sahe-se para a lista das listas. Ficar dentro
  // mostraria vista vazia sem dizer porque.
  mostra_rois();
  return true;
}

bool Navegador::junta_ao_rol(const std::string& caminho) {
  // Fóra de uma lista, `rol_corrente_` é zero, e a camada de baixo recusa o zero
  // pela chave estrangeira: rowid do SQLite parte de um, donde lista de id zero não
  // existe nunca. Guarda propria houve, e sahiu por codigo morto: a mutação que a
  // tirava sobrevivia á bateria, porque a chave já fazia o serviço.
  if (roleiro_ == nullptr) return false;
  if (!roleiro_->junta(rol_corrente_, caminho)) return false;
  refaz_vista();  // estando-se dentro d'ella, a faixa nova apparece
  return true;
}

bool Navegador::retira_do_rol() {
  if (roleiro_ == nullptr || secao_ != Secao::NoRol || vista_.empty())
    return false;
  // A ORDEM vem da columna do numero, que refaz_vista encheu com a ordem mais um.
  // Não vem do indice do eleito: com filtro posto, o indice da vista e a ordem no
  // banco desencontram-se, e retirar-se-hia a faixa errada.
  if (!roleiro_->retira(rol_corrente_, vista_[eleito_].numero - 1)) return false;
  refaz_vista();
  return true;
}

bool Navegador::sobe_no_rol() {
  if (roleiro_ == nullptr || secao_ != Secao::NoRol || vista_.empty())
    return false;
  const int ordem = vista_[eleito_].numero - 1;
  if (ordem <= 0) return false;  // o primeiro não sobe
  if (!roleiro_->troca(rol_corrente_, ordem, ordem - 1)) return false;
  if (eleito_ > 0) --eleito_;  // o olho segue a faixa que se moveu
  refaz_vista();
  return true;
}

bool Navegador::desce_no_rol() {
  if (roleiro_ == nullptr || secao_ != Secao::NoRol || vista_.empty())
    return false;
  const int ordem = vista_[eleito_].numero - 1;
  if (!roleiro_->troca(rol_corrente_, ordem, ordem + 1)) return false;
  if (eleito_ + 1 < vista_.size()) ++eleito_;
  refaz_vista();
  return true;
}

void Navegador::mostra_catalogo(nucleo::Catalogo catalogo) {
  catalogo_ = std::move(catalogo);
  secao_ = Secao::Lista;
  trilha_.clear();
  termo_.clear();
  eleito_ = 0;
  refaz_vista();
}

const std::string& Navegador::nome_do_catalogo() const noexcept {
  return catalogo_.nome;
}

const std::vector<nucleo::FaixaDoCatalogo>& Navegador::faixas_do_catalogo()
    const noexcept {
  return catalogo_.faixas;
}

bool Navegador::ha_faixa_de_catalogo() const {
  if (secao_ != Secao::Lista || vista_.empty()) return false;
  const int indice = vista_[eleito_].numero - 1;
  return indice >= 0 &&
         static_cast<std::size_t>(indice) < catalogo_.faixas.size();
}

nucleo::FaixaDoCatalogo Navegador::faixa_de_catalogo_eleita() const {
  if (!ha_faixa_de_catalogo()) return {};
  return catalogo_.faixas[static_cast<std::size_t>(vista_[eleito_].numero - 1)];
}

void Navegador::mostra_rede(std::vector<nucleo::Achado> achados) {
  achados_ = std::move(achados);
  rede_.clear();
  rede_.reserve(achados_.size());
  for (std::size_t i = 0; i < achados_.size(); ++i) {
    const nucleo::Achado& achado = achados_[i];
    Linha linha;
    // A faixa CANONICA quando a fonte a deu; o titulo do video quando não. E o
    // artista pela mesma regra, cahindo ao canal, que é o que a busca comum tem.
    linha.texto = achado.faixa.empty() ? achado.titulo : achado.faixa;
    linha.chave = achado.url;
    linha.numero = achado.numero;
    linha.duracao = achado.duracao;
    linha.autor = achado.artista.empty() ? achado.canal : achado.artista;
    linha.origem = static_cast<int>(i);
    rede_.push_back(std::move(linha));
  }
  secao_ = Secao::Rede;
  // A trilha vae-se: ella dizia por onde se andou no acervo, e a rede não está no
  // acervo. Deixá-la de pé faria o titulo da tabella mentir sobre a origem da lista.
  trilha_.clear();
  termo_.clear();
  eleito_ = 0;
  refaz_vista();
}

bool Navegador::ha_achado() const {
  if (secao_ != Secao::Rede || vista_.empty()) return false;
  const int origem = vista_[eleito_].origem;
  return origem >= 0 && static_cast<std::size_t>(origem) < achados_.size();
}

nucleo::Achado Navegador::achado_eleito() const {
  if (!ha_achado()) return {};
  return achados_[static_cast<std::size_t>(vista_[eleito_].origem)];
}

bool Navegador::entra() {
  if (vista_.empty()) return false;
  const Linha degrau = vista_[eleito_];  // CÓPIA: refaz_vista limpa a vista
  switch (secao_) {
    case Secao::Artistas:
      trilha_ = {degrau.chave};
      secao_ = Secao::Albuns;
      break;
    case Secao::Albuns:
      trilha_ = {trilha_.front(), degrau.chave};
      secao_ = Secao::Faixas;
      break;
    case Secao::Faixas:
    case Secao::Busca:
      return true;  // já é faixa: quem chama manda tocar
    case Secao::Rois:
      // Entrar n'uma lista é abri-la. O id guarda-se, e a trilha leva o nome, que
      // é o que a tela mostra por titulo.
      rol_corrente_ = std::atoi(degrau.chave.c_str());
      nome_corrente_ = degrau.texto;
      trilha_ = {degrau.texto};
      secao_ = Secao::NoRol;
      break;
    case Secao::NoRol:
      return true;  // já é faixa: quem chama enche a fila e manda tocar
    case Secao::Lista:
      // Faixa do catalogo não está no disco, e entrar n'ella não desce degrau: quem
      // chama pergunta pela eleita e manda BAIXAR. O caminho eleito fica vazio n'esta
      // secção, de proposito, como na Rede.
      return false;
    case Secao::Rede:
      // Achado da rede não é faixa, e entrar n'elle não é descer degrau algum:
      // quem chama pergunta pela url_eleita e manda baixar. Nada muda aqui.
      return false;
  }
  // O termo NÃO se herda ao descer: elle filtrava a lista de cima, e applicá-lo
  // á de baixo esconderia faixas por causa de uma busca que já se cumpriu.
  termo_.clear();
  eleito_ = 0;
  refaz_vista();
  return false;
}

bool Navegador::volta() {
  switch (secao_) {
    case Secao::Faixas:
      trilha_.resize(1);
      secao_ = Secao::Albuns;
      break;
    case Secao::Albuns:
    case Secao::Busca:
      trilha_.clear();
      secao_ = Secao::Artistas;
      break;
    case Secao::Rede:
    case Secao::Rois:
    case Secao::Lista:
      trilha_.clear();
      secao_ = Secao::Artistas;
      break;
    case Secao::NoRol:
      // De dentro de uma lista volta-se á lista das listas, e não ao acervo: é o
      // degrau de que se veio. O ALVO fica: vêr é que se deixou de estar dentro.
      trilha_.clear();
      secao_ = Secao::Rois;
      break;
    case Secao::Artistas:
      return false;  // já se está no alto
  }
  termo_.clear();
  eleito_ = 0;
  refaz_vista();
  return true;
}

bool Navegador::vai_para(Secao alvo) {
  switch (alvo) {
    case Secao::Artistas:
    case Secao::Busca:
      // O topo do acervo, e a busca n'elle inteiro: sempre ha chão. A Busca
      // com termo vazio é o acervo plano, e o filtro refina-a d'ahi.
      trilha_.clear();
      break;
    case Secao::Albuns:
      // A trilha só carrega ARTISTA quando a secção é do acervo: em NoRol o
      // primeiro degrau d'ella é o nome da lista, que artista não é.
      if (secao_ != Secao::Albuns && secao_ != Secao::Faixas) return false;
      trilha_.resize(1);
      break;
    case Secao::Faixas:
      if (secao_ != Secao::Faixas) return false;
      break;
    case Secao::Rede:
      trilha_.clear();
      break;
    case Secao::Lista:
      if (catalogo_.faixas.empty()) return false;
      trilha_.clear();
      break;
    case Secao::Rois:
      // A lista das listas sempre se abre, vazia que esteja: é o que o `P`
      // faz, e o aceite manda a barra levar ao MESMO logar.
      mostra_rois();
      return true;
    case Secao::NoRol:
      return false;  // dentro de uma lista não é degrau da barra
  }
  secao_ = alvo;
  termo_.clear();
  eleito_ = 0;
  refaz_vista();
  return true;
}

bool Navegador::vai_para_rol(int id) {
  if (roleiro_ == nullptr || id == 0) return false;
  // O nome vem do BANCO, e não da vista da barra: ella pode estar a pintar uma
  // lista que outra mão apagou entre dous quadros, e entrar n'ella mostraria
  // vista vazia sem dizer porque. Não achando o id, nada se muta.
  std::string nome;
  for (const nucleo::Rol& rol : roleiro_->rois())
    if (rol.id == id) nome = rol.nome;
  if (nome.empty()) return false;
  rol_corrente_ = id;
  nome_corrente_ = nome;
  trilha_ = {nome};
  secao_ = Secao::NoRol;
  termo_.clear();
  eleito_ = 0;
  refaz_vista();
  return true;
}

void Navegador::recarrega() {
  // Conserva a secção e a trilha SE ellas ainda existirem no acervo novo. Um
  // artista que sahiu do disco não pode continuar a ser o titulo da tabella, e
  // insistir n'elle mostraria lista vazia sem dizer porque.
  if (!trilha_.empty()) {
    const std::vector<std::string> nomes = livraria_.artistas();
    if (std::find(nomes.begin(), nomes.end(), trilha_.front()) == nomes.end()) {
      trilha_.clear();
      secao_ = Secao::Artistas;
    }
  }
  if (secao_ == Secao::Faixas && trilha_.size() == 2) {
    const std::vector<std::string> albuns = livraria_.albuns(trilha_.front());
    if (std::find(albuns.begin(), albuns.end(), trilha_.back()) == albuns.end()) {
      trilha_.resize(1);
      secao_ = Secao::Albuns;
    }
  }
  refaz_vista();
}

}  // namespace mysong::tui

//   Da lavra do eminente Doutor BRAGA US., Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
