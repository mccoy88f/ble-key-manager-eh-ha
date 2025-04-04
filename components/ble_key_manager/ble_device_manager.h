#pragma once

#include "esphome.h"
#include <vector>
#include <string>

namespace esphome {

class BLEDeviceManager : public Component {
 public:
  struct BLEDevice {
    std::string mac_address;
    std::string name;
    std::string action_id;
    int32_t last_rssi = 0;
    uint32_t last_seen = 0;
    uint32_t expiry_time = 0; // 0 = permanente, altrimenti timestamp di scadenza
  };

  BLEDeviceManager() {}

  void setup() override {
    // Carica i dispositivi salvati
    load_devices();
  }

  void loop() override {
    // Controlla le autorizzazioni scadute
    check_expired_authorizations();
  }

  // Aggiunge un nuovo dispositivo
  bool add_device(const std::string& mac_address, const std::string& name, const std::string& action_id = "") {
    // Verifica se il dispositivo esiste già
    for (auto& device : devices_) {
      if (device.mac_address == mac_address) {
        // Aggiorna il nome e l'azione se il dispositivo esiste già
        device.name = name;
        if (!action_id.empty()) {
          device.action_id = action_id;
        }
        save_devices();
        return true;
      }
    }

    // Aggiungi nuovo dispositivo
    BLEDevice device;
    device.mac_address = mac_address;
    device.name = name;
    device.action_id = action_id;
    device.last_seen = 0;
    device.last_rssi = 0;
    device.expiry_time = 0;

    devices_.push_back(device);
    save_devices();
    return true;
  }

  // Rimuove un dispositivo
  bool remove_device(const std::string& mac_address) {
    for (auto it = devices_.begin(); it != devices_.end(); ++it) {
      if (it->mac_address == mac_address) {
        devices_.erase(it);
        save_devices();
        return true;
      }
    }
    return false;
  }

  // Autorizza un dispositivo
  bool authorize_device(const std::string& mac_address, uint32_t duration_seconds = 0) {
    for (auto& device : devices_) {
      if (device.mac_address == mac_address) {
        if (duration_seconds > 0) {
          // Autorizzazione temporanea
          device.expiry_time = (millis() / 1000) + duration_seconds;
        } else {
          // Autorizzazione permanente
          device.expiry_time = 0;
        }
        save_devices();
        return true;
      }
    }
    return false;
  }

  // Revoca l'autorizzazione di un dispositivo
  bool revoke_authorization(const std::string& mac_address) {
    for (auto& device : devices_) {
      if (device.mac_address == mac_address) {
        device.expiry_time = 1; // Imposta a 1 per indicare scaduto
        save_devices();
        return true;
      }
    }
    return false;
  }

  // Verifica se un dispositivo è autorizzato
  bool is_device_authorized(const std::string& mac_address) {
    for (const auto& device : devices_) {
      if (device.mac_address == mac_address) {
        if (device.expiry_time == 0) {
          // Autorizzazione permanente
          return true;
        } else if (device.expiry_time > (millis() / 1000)) {
          // Autorizzazione temporanea ancora valida
          return true;
        }
        // Autorizzazione scaduta
        return false;
      }
    }
    // Dispositivo non trovato
    return false;
  }

  // Aggiorna l'ultima rilevazione di un dispositivo
  void update_device_seen(const std::string& mac_address, int32_t rssi) {
    for (auto& device : devices_) {
      if (device.mac_address == mac_address) {
        device.last_seen = millis() / 1000;
        device.last_rssi = rssi;
        return;
      }
    }
  }

  // Ottiene tutti i dispositivi
  std::vector<BLEDevice> get_all_devices() {
    return devices_;
  }

  // Ottiene un dispositivo specifico
  BLEDevice* get_device(const std::string& mac_address) {
    for (auto& device : devices_) {
      if (device.mac_address == mac_address) {
        return &device;
      }
    }
    return nullptr;
  }

 private:
  std::vector<BLEDevice> devices_;

  // Carica i dispositivi salvati
  void load_devices() {
    // Implementazione del caricamento da memoria persistente
    // Esempio: caricamento da file o preferenze
    // Per semplicità, qui non implementato completamente
  }

  // Salva i dispositivi
  void save_devices() {
    // Implementazione del salvataggio in memoria persistente
    // Esempio: salvataggio su file o preferenze
    // Per semplicità, qui non implementato completamente
  }

  // Controlla le autorizzazioni scadute
  void check_expired_authorizations() {
    uint32_t current_time = millis() / 1000;
    for (auto& device : devices_) {
      if (device.expiry_time > 0 && device.expiry_time <= current_time) {
        // Autorizzazione scaduta, imposta a 1 per indicare scaduto
        device.expiry_time = 1;
      }
    }
  }
};

} // namespace esphome