#pragma once
#include "platform/tc002/contract/tc002_layout.h"

#include <array>
#include <cstddef>
#include <memory>
#include <string>

#include "platform/linux/LinuxBoard.h"
#include "platform/linux/LinuxPerformanceReport.h"

namespace awtrix {

// The transport seam lets host tests exercise panel failures without device nodes.
class Tc002PanelIo {
 public:
  virtual ~Tc002PanelIo() = default;
  virtual bool acquire(std::string& error) = 0;
  virtual void setTiming(LinuxPanelTiming*) {}
  virtual std::ptrdiff_t transfer(const uint8_t* bytes, std::size_t size,
                                std::string& error) = 0;
  virtual void release() noexcept = 0;
};

class Tc002Board final : public LinuxBoard {
 public:
  static constexpr int kWidth = TC002_PANEL_WIDTH;
  static constexpr int kHeight = TC002_PANEL_HEIGHT;
  static constexpr std::size_t kRowBytes = TC002_PANEL_ROW_BYTES;
  static constexpr std::size_t kFrameBytes = kRowBytes * kHeight;
  static const render::OutputTable kPanelLight;

  Tc002Board();
  explicit Tc002Board(std::unique_ptr<Tc002PanelIo> io);
  ~Tc002Board() override;
  const char* name() const override { return "TC002"; }
  bool displayReady() const override { return ready_; }
  const std::string& error() const { return error_; }
  void setTiming(LinuxPanelTiming* timing) { timing_ = timing; }
  void begin() override;
  // Continuous output: each call sends one complete frame and exposes the
  // previously buffered frame.
  //
  // NOTE (custom build, see below): a frame that is byte-identical to the last
  // transferred one is NOT sent again. The TC002 panel MCU builds low levels by
  // repeating codes over time ("the MCU applies its own gamma and repeats codes"),
  // so it needs a *standing* image. Resending 40 identical frames per second
  // starves that repeat and makes the panel visibly brighter than the vendor app,
  // which wrote the panel only when the content changed (~once a minute when idle).
  void show(const Canvas& canvas) override;
  void setBrightness(uint8_t brightness) override;
  void applyColorGrade(const render::GradeParams& grade) override;

 private:
  bool submit(unsigned transfers);
  void fail();

  std::unique_ptr<Tc002PanelIo> io_;
  std::array<uint8_t, kFrameBytes> frame_{};
  // Letzter tatsächlich übertragener Frame (für den Vergleich in show()).
  std::array<uint8_t, kFrameBytes> shown_{};
  bool hasShown_ = false;
  LinuxPanelTiming* timing_ = nullptr;
  render::ColorGrade grade_;
  std::string error_;
  bool ready_ = false;
  bool acquired_ = false;
};

}
