"""BLE Key Manager component for ESPHome."""

import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import web_server_base
from esphome.const import CONF_ID

AUTO_LOAD = ['web_server_base']
DEPENDENCIES = ['web_server_base', 'ble_scanner']

ble_key_manager_ns = cg.esphome_ns.namespace('esphome')
BLEDeviceManager = ble_key_manager_ns.class_('BLEDeviceManager', cg.Component)
BLEWebInterface = ble_key_manager_ns.class_('BLEWebInterface', cg.Component)

CONF_BLE_DEVICE_MANAGER = 'ble_device_manager'
CONF_WEB_INTERFACE = 'web_interface'

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(CONF_BLE_DEVICE_MANAGER): cv.declare_id(BLEDeviceManager),
    cv.Optional(CONF_WEB_INTERFACE): cv.use_id(web_server_base.WebServerBase),
})

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_BLE_DEVICE_MANAGER])
    await cg.register_component(var, config)
    
    if CONF_WEB_INTERFACE in config:
        web_server = await cg.get_variable(config[CONF_WEB_INTERFACE])
        web_interface = cg.new_Pvariable(BLEWebInterface, var)
        await cg.register_component(web_interface, config)