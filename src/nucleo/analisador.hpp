// ══════════════════════════════════════════════════════════════════════════
//   TRACTADO DO ANALISADOR, src/nucleo/analisador.hpp
// ══════════════════════════════════════════════════════════════════════════
// Colhe o som da PROPRIA musica, e de mais nada. Acha no grafo do PipeWire o nó
// que o nosso mpv creou, e taponeia as portas de SAHIDA d'aquelle nó, com um
// segundo enlace em paralelo ao que já vae ao alto-falante. Notificação do
// systema, som de navegador e alerta de terminal não entram no espectro por
// CONSTRUCÇÃO, e não por filtro: elles são outros nós, e nós não os olhamos.
//
// DOMÍNIO ......... o grafo do PipeWire, tal como elle está: com nó, sem nó,
//                   com dous nós de mpv, com o serviço morto, e com o nó a
//                   morrer no meio de uma leitura.
// CONTRA-DOMÍNIO .. as bandas que se pediram, em [0,1], a qualquer instante,
//                   por bandas(). Zeros enquanto não houver nó.
// INVARIANTE ...... o nó se acha pela IDENTIDADE e não pelo nome: o cliente do
//                   PipeWire cujo application.process.id é o nosso getpid(),
//                   porque a libmpv toca DENTRO do nosso processo. Nome «mpv»
//                   ha muitos na machina de quem ouve musica. E o alvo desce
//                   pelo object.serial, NUNCA pelo object.id: medido n'esta
//                   Casa que o gestor de sessão ignora o id e nos liga ao
//                   MICROPHONE, que se move com a musica por vasamento
//                   acustico e engana a prova ingenua.
// Q.E.D. .......... a prova é uma NEGATIVA: com a musica a tocar, dispara-se
//                   notify-send e um segundo tocador em 6000 Hz, e a banda de
//                   6000 Hz não se move. Medido antes de haver codigo:
//                   mag6000 = 0,00001 com intruso e sem elle.
// ══════════════════════════════════════════════════════════════════════════
#pragma once

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

namespace mysong::nucleo {

// QUANTAS bandas a fonte entrega A QUEM NADA PEDE. Vinte e quatro, que é o que
// cabe em oitenta columnas de terminal com folga e dá 2,8 bandas por octava nas
// 8,6 octavas que vão de 40 Hz a 16 kHz, aproximadamente como o ouvido as
// espaça. Deixou de ser numero do contracto e passou a ser omissão: quem
// desenha sabe a largura que tem, e pede as bandas que vae pintar.
//
// Vive AQUI, junto do contracto, e não junto da FFT: é numero da INTERFACE, e
// quem consome bandas não precisa saber que ha transformada por baixo.
inline constexpr std::size_t QUANTAS_BANDAS = 24;

// Os limites do que se pode pedir. Abaixo de oito não ha fita: sobram columnas
// grossas que não desenham musica alguma. Acima de cento e vinte e oito não ha
// raia que chegue, que a janela de 2048 abre 1024 raias e as bandas do baixo já
// valem uma raia cada; pedir mais seria dar bandas VAZIAS, que na tela são
// columnas mortas.
inline constexpr std::size_t BANDAS_MINIMAS = 8;
inline constexpr std::size_t BANDAS_MAXIMAS = 128;

// O que um pedido VIRA, dito em função pura para que quem pede o possa saber
// sem construir espectro algum: é assim que o desenho confere se o numero que a
// largura lhe deu é o mesmo que já está em voo, e não zera o estado á toa.
std::size_t cinge_bandas(std::size_t quantas) noexcept;

// A FONTE DAS BANDAS, abstracta. É por este ponto de substituição que o tocador
// serve o espectro sem conhecer FFT nem PipeWire, e que a bateria prova a
// delegação com um dublê em machina surda.
//
// A direcção do saber é o que esta interface compra: o analisador precisa saber
// que existe um mpv; o tocador NÃO precisa saber que existe transformada.
class FonteDeBandas {
 public:
  virtual ~FonteDeBandas() = default;
  FonteDeBandas(const FonteDeBandas&) = delete;
  FonteDeBandas& operator=(const FonteDeBandas&) = delete;

  // Sempre as bandas que se pediram, sempre em [0,1]. Jamais falha: quem não tem
  // nó lê zeros, e não erro.
  virtual std::vector<float> bandas() const = 0;

  // Uma batida do relogio de quem chama. Vazia por omissão, para que o dublê da
  // bateria nada precise implementar: é na carne que o relogio de guarda vive.
  virtual void pulsa() {}

  // O PEDIDO de quem desenha: tantas bandas quantas barras a fita vae pintar.
  // Vazia por omissão pela mesma razão que pulsa(): o dublê da bateria entrega
  // o que lhe puserem, e nada tem que recalcular. Cinge-se por cinge_bandas.
  virtual void quer_bandas(std::size_t quantas) { (void)quantas; }

 protected:
  FonteDeBandas() = default;
};

// O ANALISADOR de carne, sobre a libpipewire. Nasce INERTE quando o PipeWire
// não responde, e inerte é estado legitimo e não erro: bandas em zero, razão
// legivel, e o resto do programa a correr igual. É por isso que não ha fabrica
// com optional aqui, ao contrario do MotorMpv: motor que não abre não serve
// para nada, analisador que não abre serve para dar zeros.
//
// Todo o PipeWire mora atraz do punho, e nenhuma linha d'elle sahe por este
// cabeçalho: quem inclue o analisador não herda pipewire.h, e a bateria compila
// sem os directorios de inclusão do PipeWire.
class Analisador final : public FonteDeBandas {
 public:
  Analisador();
  ~Analisador() override;

  // Falso quando o PipeWire não respondeu. Ver razao().
  bool vivo() const noexcept;
  const std::string& razao() const noexcept;

  std::vector<float> bandas() const override;

  // O relogio de guarda: passado o prazo sem buffer novo, as bandas esmorecem.
  // O nó do mpv morre SEM AVISO, e sem esta batida a ultima janela ficaria
  // congelada na tela para sempre.
  void pulsa() override;

  // O pedido do desenho, repassado ao espectro debaixo da tranca que esta Casa
  // já tem: o espectro é conta de uma linha só, e mexer-lhe nas bordas fóra da
  // fechadura seria cruzar-se com o callback de processo a alimentá-lo.
  void quer_bandas(std::size_t quantas) override;

  // O object.serial do nó a que estamos presos, e zero quando nenhum. Serve á
  // prova: é como ella confirma que nos prendemos ao nó do NOSSO processo.
  unsigned long long no() const noexcept;

 private:
  struct Punho;
  std::unique_ptr<Punho> punho_;
};

}  // namespace mysong::nucleo

// ══════════════════════════════════════════════════════════════════════════
//   Da lavra do eminente Doutor BRAGA US, Professor de Sciências Mathemáticas
//   e Geómetra desta Casa. Manuscripto lavrado no Anno da Graça de MDCCCXCVIII.
//, Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
