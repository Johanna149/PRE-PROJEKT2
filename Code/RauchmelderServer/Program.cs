using Microsoft.EntityFrameworkCore;
using Microsoft.EntityFrameworkCore.Sqlite;
using RauchmelderServer;
using RauchmelderServer.Models;

var builder = WebApplication.CreateBuilder(args);

// Lauscht auf allen Netzwerk-Interfaces, nicht nur localhost
builder.WebHost.UseUrls("http://0.0.0.0:5297");

// Add services to the container.
builder.Services.AddDbContext<SensorDbContext>(options =>
    options.UseSqlite("Data Source=sensor.db"));
builder.Services.AddControllers();
builder.Services.AddEndpointsApiExplorer();
builder.Services.AddSwaggerGen();

var app = builder.Build();

// Configure the HTTP request pipeline.
if (app.Environment.IsDevelopment())
{
    app.UseSwagger();
    app.UseSwaggerUI();
}

app.UseAuthorization();

// API Endpunkte für Sensor-Daten
app.MapPost("/api/sensor-data", async (SensorData data, SensorDbContext db) =>
{
    // Ignoriere eine mitgesendete Id, erzwinge Neu-Anlage
    data.Id = 0;
    data.Timestamp = DateTime.UtcNow;
    db.SensorData.Add(data);
    await db.SaveChangesAsync();
    return Results.Created($"/api/sensor-data/{data.Id}", data);
});

app.MapGet("/api/sensor-data", async (SensorDbContext db) =>
{
    var sensorData = await db.SensorData.OrderByDescending(s => s.Timestamp).ToListAsync();
    return Results.Ok(sensorData);
});

// Debug: Anzahl der gespeicherten Einträge
app.MapGet("/api/sensor-data/count", async (SensorDbContext db) =>
{
    var cnt = await db.SensorData.CountAsync();
    return Results.Ok(new { count = cnt });
});

// Debug: Füge Test-Einträge hinzu (nur zum Testen)
app.MapPost("/api/sensor-data/add-samples", async (SensorDbContext db) =>
{
    var now = DateTime.UtcNow;
    var list = new List<SensorData>
    {
        new SensorData{ DeviceId = "test", AdcRaw = 100, Voltage = 450, Vout = 0.5f, Rs = 1.1f, Ratio = 4.2f, Timestamp = now },
        new SensorData{ DeviceId = "test", AdcRaw = 110, Voltage = 455, Vout = 0.55f, Rs = 1.15f, Ratio = 4.3f, Timestamp = now.AddSeconds(1) },
        new SensorData{ DeviceId = "test", AdcRaw = 120, Voltage = 460, Vout = 0.6f, Rs = 1.2f, Ratio = 4.4f, Timestamp = now.AddSeconds(2) }
    };

    db.SensorData.AddRange(list);
    await db.SaveChangesAsync();
    return Results.Created("/api/sensor-data/sample", list);
});

app.MapGet("/api/sensor-data/{id}", async (int id, SensorDbContext db) =>
{
    var sensorData = await db.SensorData.FindAsync(id);
    return sensorData is not null ? Results.Ok(sensorData) : Results.NotFound();
});

app.MapDelete("/api/sensor-data/{id}", async (int id, SensorDbContext db) =>
{
    var sensorData = await db.SensorData.FindAsync(id);
    if (sensorData is null)
    {
        return Results.NotFound();
    }

    db.SensorData.Remove(sensorData);
    await db.SaveChangesAsync();
    return Results.NoContent();
});

// Erstelle Datenbank falls nicht vorhanden
using (var scope = app.Services.CreateScope())
{
    var db = scope.ServiceProvider.GetRequiredService<SensorDbContext>();
    db.Database.EnsureCreated();
}

<<<<<<< HEAD
// Einfache Dashboard-HTML-Seite zur Anzeige der Messwerte
app.MapGet("/", () =>
{
        var html = @"<!doctype html>
<html>
    <head>
        <meta charset='utf-8' />
        <title>Sensor Dashboard</title>
        <style>
            body{font-family:Segoe UI,Arial;background:#111;color:#eee;padding:16px}
            table{border-collapse:collapse;width:100%;max-width:1000px}
            th,td{padding:8px;border-bottom:1px solid #333;text-align:left}
            th{background:#0b3}
            .muted{color:#9aa}
        </style>
    </head>
    <body>
        <h1>Sensor Dashboard</h1>
        <div id='count' class='muted'></div>
        <table id='tbl'>
            <thead><tr><th>Id</th><th>Device</th><th>ADC</th><th>Voltage</th><th>Vout</th><th>Rs</th><th>Ratio</th><th>Timestamp (UTC)</th></tr></thead>
            <tbody></tbody>
        </table>
        <script>
            async function load(){
                const res=await fetch('/api/sensor-data');
                const data=await res.json();
                const tbody=document.querySelector('#tbl tbody');
                tbody.innerHTML='';
                data.forEach(d=>{
                    const tr=document.createElement('tr');
                    tr.innerHTML=`<td>${d.id}</td><td>${d.device_id}</td><td>${d.adc_raw}</td><td>${d.voltage}</td><td>${d.vout}</td><td>${d.rs}</td><td>${d.ratio}</td><td>${new Date(d.timestamp).toLocaleString()}</td>`;
                    tbody.appendChild(tr);
                });
                document.getElementById('count').textContent=`${data.length} Einträge`;
            }
            load();
            setInterval(load,5000);
        </script>
    </body>
</html>";

        return Results.Content(html, "text/html");
});

app.Run();
=======
app.Run("http://0.0.0.0:5297");
>>>>>>> d98641119efe69b3f69e851c998cdf0575e19244
