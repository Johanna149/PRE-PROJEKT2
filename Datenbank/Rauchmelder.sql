-- Optional: alte Tabellen löschen
DROP TABLE IF EXISTS speichert_werte CASCADE;
DROP TABLE IF EXISTS melder CASCADE;

-- Tabelle: Melder
CREATE TABLE melder (
    id SERIAL PRIMARY KEY,
    bezeichnungen TEXT NOT NULL
);

-- Tabelle: Speichert Werte
CREATE TABLE speichert_werte (
    id SERIAL PRIMARY KEY,
    melder_id INTEGER NOT NULL,
    datum DATE NOT NULL,
    uhrzeit TIME NOT NULL,
    status VARCHAR(50) NOT NULL,

    CONSTRAINT fk_melder
        FOREIGN KEY (melder_id)
        REFERENCES melder(id)
        ON DELETE CASCADE
);

-- Optional sinnvolle Indizes
CREATE INDEX idx_speichert_werte_melder_id ON speichert_werte(melder_id);
CREATE INDEX idx_speichert_werte_datum ON speichert_werte(datum);
CREATE INDEX idx_speichert_werte_status ON speichert_werte(status);
