// ══════════════════════════════════════════════════════════════════════════
//   TRACTADO DO MOTOR, src/nucleo/motor.hpp
// ══════════════════════════════════════════════════════════════════════════
// Declara o MOTOR: a potencia que faz sahir som de um arquivo, e as ordens a
// que ella responde. Não é o tocador, que governa a fila; é só a potencia.
// Declara-se ABSTRACTA de proposito, para que a bateria de provas ponha em seu
// logar um dublê que não abre placa de som alguma.
//
// DOMÍNIO ......... um caminho de arquivo no systema, e ordens de operador:
//                   tocar, pausar, retomar, buscar, volume.
// CONTRA-DOMÍNIO .. som na saída de áudio, e tres grandezas legíveis a
//                   qualquer instante: posição, duração e estado.
// INVARIANTE ...... o motor NÃO toca o volume do systema. O volume d'aqui é o
//                   do proprio motor, e o contracto declarado de vol.sh fica
//                   intacto. Nem governa fila: uma faixa de cada vez, e a
//                   ordem é de quem chama, nunca do motor.
// Q.E.D. .......... sendo a interface abstracta, a prova da fila e das
//                   transições corre em máquina surda; e sendo o volume o do
//                   motor e não o do systema, provar que o systema não mudou
//                   reduz-se a medi-lo antes e depois.
// ══════════════════════════════════════════════════════════════════════════
#pragma once

#include <functional>
#include <optional>
#include <string>
#include <string_view>

// O punho da libmpv, declarado adiante e não incluido: assim mpv/client.h não
// entra por este cabecalho em unidade alguma que não precise d'elle, e a
// bateria de provas compila sem os directorios de inclusão do mpv.
extern "C" {
struct mpv_handle;
}

namespace mysong::nucleo {

// O estado do motor. Tres, e não mais: quem não toca nem pausa, está parado.
enum class Estado { Parado, Tocando, Pausado };

// O nome do estado, para relatorio de prova e para olho humano. Inline, e não
// em unidade de traducção: assim a bateria o lê sem linkar a libmpv, que é
// quem mora no motor.cpp.
inline std::string_view nome_do_estado(Estado estado) noexcept {
  switch (estado) {
    case Estado::Tocando: return "Tocando";
    case Estado::Pausado: return "Pausado";
    case Estado::Parado:  break;
  }
  return "Parado";
}

// O APARO das bordas, em fonte UNICA. Ordem de operador humano fóra de faixa é
// rotina e não catastrophe: apara-se, e não se ergue excepção, porque erguer
// obrigaria toda tecla de toda tela futura a envolver-se em try.
inline int aparar_volume(int porcento) noexcept {
  if (porcento < 0) return 0;
  if (porcento > 100) return 100;
  return porcento;
}

// Buscar além do fim apara-se ao fim menos uma folga, para que o alvo caia
// DENTRO do arquivo: mandar ao mpv um alvo fóra d'elle é pedir-lhe o fim da
// faixa, que é resposta differente da que o operador pediu.
inline double aparar_busca(double alvo, double duracao) noexcept {
  const double folga = 0.05;
  if (alvo < 0.0) return 0.0;
  if (duracao > folga && alvo > duracao - folga) return duracao - folga;
  return alvo;
}

// O que se annuncia a quem escuta. Quatro avisos, e nenhum d'elles carrega
// quem os ouve: o nucleo emitte ao vento, e quem quiser recolhe.
enum class Aviso { FaixaMudou, EstadoMudou, PosicaoAndou, FalhouAoTocar };

// O pregão: o aviso, e o retracto do mundo no instante em que se deu. Vae por
// valor e completo, de sorte que o ouvinte nada precise interrogar de volta,
// interrogar de volta seria o ouvinte conhecer o nucleo, e não sómente o
// nucleo ignorar o ouvinte.
struct Evento {
  Aviso aviso = Aviso::EstadoMudou;
  Estado estado = Estado::Parado;
  std::string faixa;        // vazia quando nenhuma faixa esta em curso
  double posicao = 0.0;     // em segundos, contados do inicio da faixa
  std::string razao;        // preenchida sómente em FalhouAoTocar
};

// Quem escuta. Zero ouvintes é caso legitimo e não erro: a emissão ao vento,
// sem ninguem que a recolha, é o estado normal da bateria de provas.
using Ouvinte = std::function<void(const Evento&)>;

// A POTENCIA, abstracta. É por este ponto de substituição que a bateria põe um
// dublê no logar do mpv; sem elle, a prova da fila exigiria placa de som, e o
// aceite que pede motor dublê não teria onde se prender.
//
// Toda ordem devolve se foi acceita. Toda leitura é const e jamais falha: quem
// nada toca lê posição zero, e não erro.
class Motor {
 public:
  virtual ~Motor() = default;
  Motor(const Motor&) = delete;
  Motor& operator=(const Motor&) = delete;

  virtual bool tocar(const std::string& caminho) = 0;
  virtual bool pausar() = 0;
  virtual bool retomar() = 0;
  virtual bool buscar(double segundos) = 0;
  virtual bool volume(int porcento) = 0;

  virtual double posicao() const = 0;
  virtual double duracao() const = 0;
  virtual Estado estado() const = 0;

  // Drena o que a potencia tiver a dizer. Chama-se de fóra, em cadencia de
  // quem chama: o motor não cria linha de execução propria.
  virtual void bombear() = 0;
  // Consome apenas o fim natural; parar ou falhar não autoriza avançar.
  virtual bool consome_fim_natural() { return false; }

 protected:
  Motor() = default;
};

// A POTENCIA DE CARNE, sobre a libmpv. Nasce SÓ pela fabrica: não ha
// construtor publico, e por isso o motor invalido não é exprimivel. Quem não
// conseguiu abrir não tem objecto, e não um objecto a que se deva perguntar se
// serve; a pergunta que ninguem faz é o defeito que apparece longe da causa.
class MotorMpv final : public Motor {
 public:
  // Vazio se a libmpv recusar. A razão, se se pedir, sahe legivel por olho.
  static std::optional<MotorMpv> abrir(std::string* razao = nullptr);

  // A versão da interface da libmpv com que se compilou, para relatorio.
  static unsigned long versao_da_interface() noexcept;

  ~MotorMpv() override;

  // Move consentido, e sómente elle: é o que a fabrica precisa para devolver.
  MotorMpv(MotorMpv&& outro) noexcept;
  MotorMpv& operator=(MotorMpv&&) = delete;

  bool tocar(const std::string& caminho) override;
  bool pausar() override;
  bool retomar() override;
  bool buscar(double segundos) override;
  bool volume(int porcento) override;

  double posicao() const override;
  double duracao() const override;
  Estado estado() const override;
  void bombear() override;
  bool consome_fim_natural() override;

  // JANELLA DE LEITURA: qualquer propriedade do mpv, em texto, e cadeia vazia
  // quando ella não existe. Só LÊ, e nunca escreve, de sorte que abri-la não
  // dá a ninguem poder que a interface do Motor já não desse. É por ella que a
  // prova assere «playlist-count» e demonstra que a fila não desceu ao mpv.
  std::string propriedade(const char* nome) const;

 private:
  explicit MotorMpv(::mpv_handle* punho) noexcept;

  ::mpv_handle* punho_ = nullptr;
  Estado estado_ = Estado::Parado;
  double posicao_ = 0.0;
  double duracao_ = 0.0;
  bool fim_natural_ = false;
};

}  // namespace mysong::nucleo

// ══════════════════════════════════════════════════════════════════════════
//   Da lavra do eminente Doutor BRAGA US, Professor de Sciências Mathemáticas
//   e Geómetra desta Casa. Manuscripto lavrado no Anno da Graça de MDCCCXCVIII.
//, Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
