#pragma once

#include "esphome.h"
#include "ble_device_manager.h"

namespace esphome {

class BLEWebInterface : public Component {
 public:
  BLEWebInterface(BLEDeviceManager *device_manager) : device_manager_(device_manager) {}

  void setup() override {
    // Registra gli endpoint dell'API web
    register_web_handlers();
  }

 private:
  BLEDeviceManager *device_manager_;

  // Registra gli handler per le richieste web
  void register_web_handlers() {
    // Pagina principale
    App.get_web_server()->on("/", HTTP_GET, [this](AsyncWebServerRequest *request) {
      if (!request->authenticate("admin", "password")) { // Usa credenziali dal file secrets.yaml
        return request->requestAuthentication();
      }
      
      AsyncResponseStream *response = request->beginResponseStream("text/html");
      response->print(F("<!DOCTYPE html><html><head>"));
      response->print(F("<meta charset=\"UTF-8\">"));
      response->print(F("<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">"));
      response->print(F("<title>BLE Key Manager</title>"));
      response->print(F("<style>"));
      response->print(F("body{font-family:Arial,sans-serif;margin:0;padding:20px;line-height:1.6;}"));
      response->print(F("h1{color:#333;}"));
      response->print(F(".container{max-width:1200px;margin:0 auto;}"));
      response->print(F(".card{background:#f9f9f9;border-radius:5px;padding:15px;margin-bottom:15px;box-shadow:0 2px 4px rgba(0,0,0,0.1);}"));
      response->print(F(".device{display:flex;justify-content:space-between;align-items:center;}"));
      response->print(F(".device-info{flex:1;}"));
      response->print(F(".device-actions{display:flex;gap:10px;}"));
      response->print(F("button{background:#4CAF50;color:white;border:none;padding:8px 12px;border-radius:4px;cursor:pointer;}"));
      response->print(F("button.revoke{background:#f44336;}"));
      response->print(F("button.edit{background:#2196F3;}"));
      response->print(F(".add-form{margin-top:20px;}"));
      response->print(F("input,select{padding:8px;margin-right:10px;border-radius:4px;border:1px solid #ddd;}"));
      response->print(F(".authorized{color:green;font-weight:bold;}"));
      response->print(F(".unauthorized{color:red;}"));
      response->print(F("</style>"));
      response->print(F("</head><body>"));
      response->print(F("<div class=\"container\">"));
      response->print(F("<h1>BLE Key Manager</h1>"));
      
      // Sezione dispositivi
      response->print(F("<h2>Dispositivi BLE</h2>"));
      
      auto devices = device_manager_->get_all_devices();
      if (devices.empty()) {
        response->print(F("<p>Nessun dispositivo registrato.</p>"));
      } else {
        for (const auto &device : devices) {
          response->print(F("<div class=\"card\"><div class=\"device\">"));
          
          // Informazioni dispositivo
          response->print(F("<div class=\"device-info\">"));
          response->printf(F("<h3>%s</h3>"), device.name.c_str());
          response->printf(F("<p>MAC: %s</p>"), device.mac_address.c_str());
          
          // Stato autorizzazione
          if (device_manager_->is_device_authorized(device.mac_address)) {
            response->print(F("<p class=\"authorized\">Autorizzato"));
            
            // Mostra scadenza se presente
            if (device.expiry_time > 0) {
              uint32_t current_time = millis() / 1000;
              uint32_t remaining = (device.expiry_time > current_time) ? 
                                   (device.expiry_time - current_time) : 0;
              
              uint32_t hours = remaining / 3600;
              uint32_t minutes = (remaining % 3600) / 60;
              uint32_t seconds = remaining % 60;
              
              response->printf(F(" (scade tra %02u:%02u:%02u)"), hours, minutes, seconds);
            }
            
            response->print(F("</p>"));
          } else {
            response->print(F("<p class=\"unauthorized\">Non autorizzato</p>"));
          }
          
          // Azione associata
          if (!device.action_id.empty()) {
            response->printf(F("<p>Azione: %s</p>"), device.action_id.c_str());
          } else {
            response->print(F("<p>Nessuna azione definita</p>"));
          }
          
          // Ultima rilevazione
          if (device.last_seen > 0) {
            uint32_t current_time = millis() / 1000;
            uint32_t seconds_ago = current_time - device.last_seen;
            
            if (seconds_ago < 60) {
              response->printf(F("<p>Rilevato %u secondi fa (RSSI: %d dBm)</p>"), 
                             seconds_ago, device.last_rssi);
            } else if (seconds_ago < 3600) {
              response->printf(F("<p>Rilevato %u minuti fa (RSSI: %d dBm)</p>"), 
                             seconds_ago / 60, device.last_rssi);
            } else {
              response->printf(F("<p>Rilevato %u ore fa (RSSI: %d dBm)</p>"), 
                             seconds_ago / 3600, device.last_rssi);
            }
          } else {
            response->print(F("<p>Mai rilevato</p>"));
          }
          
          response->print(F("</div>"));
          
          // Azioni dispositivo
          response->print(F("<div class=\"device-actions\">"));
          
          // Pulsante autorizza/revoca
          if (device_manager_->is_device_authorized(device.mac_address)) {
            response->printf(F("<button class=\"revoke\" onclick=\"window.location.href='/revoke?mac=%s';\">Revoca</button>"), 
                           device.mac_address.c_str());
          } else {
            response->printf(F("<button onclick=\"window.location.href='/authorize?mac=%s';\">Autorizza</button>"), 
                           device.mac_address.c_str());
            response->printf(F("<button onclick=\"window.location.href='/authorize_temp?mac=%s';\">Autorizza (24h)</button>"), 
                           device.mac_address.c_str());
          }
          
          // Pulsante modifica
          response->printf(F("<button class=\"edit\" onclick=\"window.location.href='/edit?mac=%s';\">Modifica</button>"), 
                         device.mac_address.c_str());
          
          // Pulsante elimina
          response->printf(F("<button class=\"revoke\" onclick=\"if(confirm('Sei sicuro di voler eliminare questo dispositivo?')) window.location.href='/delete?mac=%s';\">Elimina</button>"), 
                         device.mac_address.c_str());
          
          response->print(F("</div></div></div>"));
        }
      }
      
      // Form per aggiungere un nuovo dispositivo
      response->print(F("<div class=\"add-form\"><h2>Aggiungi Dispositivo</h2>"));
      response->print(F("<form action=\"/add\" method=\"get\">"));
      response->print(F("<input type=\"text\" name=\"mac\" placeholder=\"Indirizzo MAC (XX:XX:XX:XX:XX:XX)\" required>"));
      response->print(F("<input type=\"text\" name=\"name\" placeholder=\"Nome dispositivo\" required>"));
      response->print(F("<select name=\"action\">"));
      response->print(F("<option value=\"\">Nessuna azione</option>"));
      response->print(F("<option value=\"toggle_relay\">Toggle Relè</option>"));
      response->print(F("<option value=\"turn_on_relay\">Accendi Relè</option>"));
      response->print(F("<option value=\"turn_off_relay\">Spegni Relè</option>"));
      response->print(F("</select>"));
      response->print(F("<button type=\"submit\">Aggiungi</button>"));
      response->print(F("</form></div>"));
      
      response->print(F("</div></body></html>"));
      request->send(response);
    });
    
    // Endpoint per aggiungere un dispositivo
    App.get_web_server()->on("/add", HTTP_GET, [this](AsyncWebServerRequest *request) {
      if (!request->authenticate("admin", "password")) {
        return request->requestAuthentication();
      }
      
      if (request->hasParam("mac") && request->hasParam("name")) {
        String mac = request->getParam("mac")->value();
        String name = request->getParam("name")->value();
        String action = request->hasParam("action") ? request->getParam("action")->value() : "";
        
        if (device_manager_->add_device(mac.c_str(), name.c_str(), action.c_str())) {
          request->redirect("/");
        } else {
          request->send(400, "text/plain", "Errore nell'aggiunta del dispositivo");
        }
      } else {
        request->send(400, "text/plain", "Parametri mancanti");
      }
    });
    
    // Endpoint per autorizzare un dispositivo
    App.get_web_server()->on("/authorize", HTTP_GET, [this](AsyncWebServerRequest *request) {
      if (!request->authenticate("admin", "password")) {
        return request->requestAuthentication();
      }
      
      if (request->hasParam("mac")) {
        String mac = request->getParam("mac")->value();
        
        if (device_manager_->authorize_device(mac.c_str())) {
          request->redirect("/");
        } else {
          request->send(400, "text/plain", "Errore nell'autorizzazione del dispositivo");
        }
      } else {
        request->send(400, "text/plain", "Parametro MAC mancante");
      }
    });
    
    // Endpoint per autorizzazione temporanea (24 ore)
    App.get_web_server()->on("/authorize_temp", HTTP_GET, [this](AsyncWebServerRequest *request) {
      if (!request->authenticate("admin", "password")) {
        return request->requestAuthentication();
      }
      
      if (request->hasParam("mac")) {
        String mac = request->getParam("mac")->value();
        
        // Autorizza per 24 ore (86400 secondi)
        if (device_manager_->authorize_device(mac.c_str(), 86400)) {
          request->redirect("/");
        } else {
          request->send(400, "text/plain", "Errore nell'autorizzazione temporanea del dispositivo");
        }
      } else {
        request->send(400, "text/plain", "Parametro MAC mancante");
      }
    });
    
    // Endpoint per revocare l'autorizzazione
    App.get_web_server()->on("/revoke", HTTP_GET, [this](AsyncWebServerRequest *request) {
      if (!request->authenticate("admin", "password")) {
        return request->requestAuthentication();
      }
      
      if (request->hasParam("mac")) {
        String mac = request->getParam("mac")->value();
        
        if (device_manager_->revoke_authorization(mac.c_str())) {
          request->redirect("/");
        } else {
          request->send(400, "text/plain", "Errore nella revoca dell'autorizzazione");
        }
      } else {
        request->send(400, "text/plain", "Parametro MAC mancante");
      }
    });
    
    // Endpoint per eliminare un dispositivo
    App.get_web_server()->on("/delete", HTTP_GET, [this](AsyncWebServerRequest *request) {
      if (!request->authenticate("admin", "password")) {
        return request->requestAuthentication();
      }
      
      if (request->hasParam("mac")) {
        String mac = request->getParam("mac")->value();
        
        if (device_manager_->remove_device(mac.c_str())) {
          request->redirect("/");
        } else {
          request->send(400, "text/plain", "Errore nell'eliminazione del dispositivo");
        }
      } else {
        request->send(400, "text/plain", "Parametro MAC mancante");
      }
    });
  }
};

} // namespace esphome