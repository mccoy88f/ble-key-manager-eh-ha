# BLE Key Manager per ESP32

Questo progetto implementa un sistema di gestione chiavi BLE per ESP32 utilizzando ESPHome. Permette di rilevare, associare e gestire dispositivi Bluetooth Low Energy, con funzionalità per autorizzarli temporaneamente e definire azioni personalizzate quando viene premuto un pulsante fisico.

## Caratteristiche

- Rilevamento automatico di dispositivi BLE nelle vicinanze
- Interfaccia web per gestire i dispositivi
- Autorizzazione temporanea o permanente dei dispositivi
- Associazione di azioni personalizzate ai dispositivi
- Esecuzione di azioni alla pressione di un pulsante fisico
- Memorizzazione persistente dei dispositivi associati

## Requisiti Hardware

- ESP32 (qualsiasi variante con supporto BLE)
- Pulsante fisico (opzionale, collegato al GPIO0)
- Relè o altro attuatore (opzionale, collegato al GPIO2)

## Installazione

1. Installa ESPHome (se non l'hai già fatto):
   ```
   pip install esphome
   ```

2. Modifica il file `secrets.yaml` con le tue credenziali WiFi e password

3. Compila e carica il firmware sull'ESP32:
   ```
   esphome run ble_key_manager.yaml
   ```

## Utilizzo

1. Dopo l'avvio, l'ESP32 creerà un server web accessibile all'indirizzo `http://ble_key_manager.local` (o all'IP assegnato dal router)

2. Accedi all'interfaccia web utilizzando le credenziali configurate nel file `secrets.yaml`

3. Nella pagina principale puoi:
   - Visualizzare i dispositivi BLE rilevati
   - Aggiungere manualmente nuovi dispositivi
   - Autorizzare o revocare l'accesso ai dispositivi
   - Impostare autorizzazioni temporanee
   - Associare azioni ai dispositivi

4. Quando premi il pulsante fisico, il sistema eseguirà l'azione associata al dispositivo BLE autorizzato più vicino

## Personalizzazione

### Aggiungere nuove azioni

Per aggiungere nuove azioni personalizzate, modifica la sezione relativa nel file `ble_key_manager.yaml`:

```yaml
on_press:
  then:
    - lambda: |-
        // ...
        if (device->action_id == "toggle_relay") {
          id(output_relay).toggle();
        } else if (device->action_id == "turn_on_relay") {
          id(output_relay).turn_on();
        } else if (device->action_id == "turn_off_relay") {
          id(output_relay).turn_off();
        } else if (device->action_id == "tua_nuova_azione") {
          // Aggiungi qui il codice per la tua nuova azione
        }
```

## Risoluzione dei problemi

- Se l'ESP32 non si connette al WiFi, si avvierà in modalità access point con SSID "BLE Key Manager Fallback"
- Per reimpostare la configurazione, cancella la memoria flash dell'ESP32
- Per il debug, aumenta il livello di log nel file `ble_key_manager.yaml`

## Licenza

Questo progetto è rilasciato sotto licenza MIT.