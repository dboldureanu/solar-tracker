use esp_idf_svc::hal::delay::FreeRtos;
use esp_idf_svc::hal::gpio::PinDriver;
use esp_idf_svc::hal::peripherals::Peripherals;
use log::info;

fn main() {
    // Initialize ESP-IDF system (logger, event loop, etc.)
    esp_idf_svc::sys::link_patches();
    esp_idf_svc::log::EspLogger::initialize_default();

    info!("Solar Tracker - Phase 1: Blink & Serial");
    info!("ESP32 Rust toolchain is working!");

    let peripherals = Peripherals::take().unwrap();

    // Onboard LED is typically on GPIO 2 for ESP32 DevKit boards
    let mut led = PinDriver::output(peripherals.pins.gpio2).unwrap();

    let mut counter: u32 = 0;
    loop {
        led.set_high().unwrap();
        info!("LED ON  (cycle {})", counter);
        FreeRtos::delay_ms(500);

        led.set_low().unwrap();
        info!("LED OFF (cycle {})", counter);
        FreeRtos::delay_ms(500);

        counter += 1;
    }
}
