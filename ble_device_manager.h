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
        // Se expiry_time è 0, l'autorizzazione è permanente
        if (device.expiry_time == 0) {
          return true;
        }
        
        // Altrimenti, controlla se l'autorizzazione è ancora valida
        uint32_t current_time = millis() / 1000;
        return device.expiry_time > current_time;
      }
    }
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

  // Ottiene un dispositivo dal MAC address
  BLEDevice* get_device(const std::string& mac_address) {
    for (auto& device : devices_) {
      if (device.mac_address == mac_address) {
        return &device;
      }
    }
    return nullptr;
  }

  // Ottiene tutti i dispositivi
  const std::vector<BLEDevice>& get_all_devices() const {
    return devices_;
  }

  // Imposta l'azione per un dispositivo
  bool set_device_action(const std::string& mac_address, const std::string& action_id) {
    for (auto& device : devices_) {
      if (device.mac_address == mac_address) {
        device.action_id = action_id;
        save_devices();
        return true;
      }
    }
    return false;
  }

 private:
  std::vector<BLEDevice> devices_;

  // Carica i dispositivi dal file system
  void load_devices() {
    devices_.clear();
    
    // Usa ESPHome Preferences per caricare i dati
    auto storage = global_preferences->make_preference<uint16_t>("ble_device_count");
    uint16_t count = 0;
    if (!storage.load(&count)) {
      ESP_LOGD("ble_manager", "Nessun dispositivo salvato");
      return;
    }
    
    ESP_LOGD("ble_manager", "Caricamento di %u dispositivi", count);
    
    for (uint16_t i = 0; i < count; i++) {
      BLEDevice device;
      char mac_key[32];
      char name_key[32];
      char action_key[32];
      char expiry_key[32];
      
      sprintf(mac_key, "ble_mac_%u", i);
      sprintf(name_key, "ble_name_%u", i);
      sprintf(action_key, "ble_action_%u", i);
      sprintf(expiry_key, "ble_expiry_%u", i);
      
      // Carica MAC address (64 caratteri max)
      auto mac_storage = global_preferences->make_preference<char[64]>(mac_key);
      char mac_buf[64] = {0};
      if (!mac_storage.load(&mac_buf)) continue;
      device.mac_address = mac_buf;
      
      // Carica nome (64 caratteri max)
      auto name_storage = global_preferences->make_preference<char[64]>(name_key);
      char name_buf[64] = {0};
      if (!name_storage.load(&name_buf)) continue;
      device.name = name_buf;
      
      // Carica action_id (64 caratteri max)
      auto action_storage = global_preferences->make_preference<char[64]>(action_key);
      char action_buf[64] = {0};
      if (action_storage.load(&action_buf)) {
        device.action_id = action_buf;
      }
      
      // Carica expiry_time
      auto expiry_storage = global_preferences->make_preference<uint32_t>(expiry_key);
      expiry_storage.load(&device.expiry_time);
      
      devices_.push_back(device);
    }
  }

  // Salva i dispositivi nel file system
  void save_devices() {
    uint16_t count = devices_.size();
    auto storage = global_preferences->make_preference<uint16_t>("ble_device_count");
    storage.save(&count);
    
    ESP_LOGD("ble_manager", "Salvataggio di %u dispositivi", count);
    
    for (uint16_t i = 0; i < count; i++) {
      const auto& device = devices_[i];
      
      char mac_key[32];
      char name_key[32];
      char action_key[32];
      char expiry_key[32];
      
      sprintf(mac_key, "ble_mac_%u", i);
      sprintf(name_key, "ble_name_%u", i);
      sprintf(action_key, "ble_action_%u", i);
      sprintf(expiry_key, "ble_expiry_%u", i);
      
      // Salva MAC address
      auto mac_storage = global_preferences->make_preference<char[64]>(mac_key);
      char mac_buf[64] = {0};
      strncpy(mac_buf, device.mac_address.c_str(), sizeof(mac_buf) - 1);
      mac_storage.save(&mac_buf);
      
      // Salva nome
      auto name_storage = global_preferences->make_preference<char[64]>(name_key);
      char name_buf[64] = {0};
      strncpy(name_buf, device.name.c_str(), sizeof(name_buf) - 1);
      name_storage.save(&name_buf);
      
      // Salva action_id
      auto action_storage = global_preferences->make_preference<char[64]>(action_key);
      char action_buf[64] = {0};
      strncpy(action_buf, device.action_id.c_str(), sizeof(action_buf) - 1);
      action_storage.save(&action_buf);
      
      // Salva expiry_time
      auto expiry_storage = global_preferences->make_preference<uint32_t>(expiry_key);
      expiry_storage.save(&device.expiry_time);
    }
  }

  // Controlla le autorizzazioni scadute
  void check_expired_authorizations() {
    uint32_t current_time = millis() / 1000;
    bool changes = false;
    
    for (auto& device : devices_) {
      // Se expiry_time è 0, l'autorizzazione è permanente
      if (device.expiry_time > 0 && device.expiry_time <= current_time) {
        // L'autorizzazione è scaduta, imposta a 1 per indicare scaduto
        if (device.expiry_time != 1) {
          device.expiry_time = 1;
          changes = true;
          ESP_LOGD("ble_manager", "Autorizzazione scaduta per %s", device.name.c_str());
        }
      }
    }
    
    if (changes) {
      save_devices();
    }
  }
};

} // namespace esphome