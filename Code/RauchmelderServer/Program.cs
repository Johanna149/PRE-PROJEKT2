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

app.UseHttpsRedirection();
app.UseAuthorization();

// API Endpunkte für Sensor-Daten
app.MapPost("/api/sensor-data", async (SensorData data, SensorDbContext db) =>
{
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

app.Run();
