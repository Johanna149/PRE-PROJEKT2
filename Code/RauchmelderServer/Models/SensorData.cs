using System.ComponentModel.DataAnnotations;

namespace RauchmelderServer.Models;

public class SensorData
{
    [Key]
    public int Id { get; set; }
    public string DeviceId { get; set; } = string.Empty;
    public int AdcRaw { get; set; }
    public int Voltage { get; set; }
    public float Vout { get; set; }
    public float Rs { get; set; }
    public float Ratio { get; set; }
    public DateTime Timestamp { get; set; } = DateTime.UtcNow;
}