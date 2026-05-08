using Microsoft.EntityFrameworkCore;
using RauchmelderServer.Models;

namespace RauchmelderServer;

public class SensorDbContext : DbContext
{
    public SensorDbContext(DbContextOptions<SensorDbContext> options)
        : base(options)
    {
    }

    public DbSet<SensorData> SensorData { get; set; }
}