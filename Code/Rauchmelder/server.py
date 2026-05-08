"""
Rauchmelder Server - Empfängt Sensordaten und speichert in PostgreSQL
"""
from flask import Flask, request, jsonify
import psycopg2
from datetime import datetime
import logging

# Konfiguration
DB_CONFIG = {
    "host": "10.0.29.177",
    "port": 5432,
    "database": "Rauchmelder",
    "user": "1234",
    "password": "1234"
}

app = Flask(__name__)
logging.basicConfig(level=logging.INFO)
logger = logging.getLogger(__name__)


def init_database():
    """Erstellt die Tabelle falls sie nicht existiert"""
    try:
        conn = psycopg2.connect(**DB_CONFIG)
        cur = conn.cursor()
        
        # Tabelle erstellen falls nicht vorhanden
        cur.execute("""
            CREATE TABLE IF NOT EXISTS sensor_data (
                id SERIAL PRIMARY KEY,
                timestamp TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
                adc_raw INTEGER,
                voltage_mv INTEGER,
                vout_v REAL,
                rs_kohm REAL,
                ratio REAL,
                device_id TEXT
            )
        """)
        
        conn.commit()
        cur.close()
        conn.close()
        logger.info("Datenbank initialisiert")
    except Exception as e:
        logger.error(f"Datenbankfehler: {e}")


@app.route('/api/sensor-data', methods=['POST'])
def receive_sensor_data():
    """Empfängt Sensordaten vom ESP32"""
    try:
        data = request.get_json()
        
        # Daten aus JSON extrahieren
        adc_raw = data.get('adc_raw')
        voltage = data.get('voltage')
        vout = data.get('vout')
        rs = data.get('rs')
        ratio = data.get('ratio')
        device_id = data.get('device_id', 'ESP32-001')
        
        # In PostgreSQL speichern
        conn = psycopg2.connect(**DB_CONFIG)
        cur = conn.cursor()
        
        cur.execute("""
            INSERT INTO sensor_data (adc_raw, voltage_mv, vout_v, rs_kohm, ratio, device_id)
            VALUES (%s, %s, %s, %s, %s, %s)
        """, (adc_raw, voltage, vout, rs, ratio, device_id))
        
        conn.commit()
        cur.close()
        conn.close()
        
        logger.info(f"Daten gespeichert: ADC={adc_raw}, Voltage={voltage}mV, Ratio={ratio}")
        
        return jsonify({"status": "success", "message": "Daten gespeichert"}), 201
        
    except Exception as e:
        logger.error(f"Fehler beim Speichern: {e}")
        return jsonify({"status": "error", "message": str(e)}), 500


@app.route('/health', methods=['GET'])
def health_check():
    """Health Check Endpoint"""
    return jsonify({"status": "healthy", "timestamp": datetime.now().isoformat()})


if __name__ == '__main__':
    init_database()
    print("Server startet auf http://0.0.0.0:5000")
    app.run(host='0.0.0.0', port=5000, debug=True)