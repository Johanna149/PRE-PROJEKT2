using System.ComponentModel.DataAnnotations;
using System.ComponentModel.DataAnnotations.Schema;
using System.Text.Json.Serialization;

namespace RauchmelderServer.Models;

public class SensorData
{
    [Key]
    [DatabaseGenerated(DatabaseGeneratedOption.Identity)]
    public int Id { get; set; }

    [JsonPropertyName("device_id")]
    public string DeviceId { get; set; } = string.Empty;

    [JsonPropertyName("adc_raw")]
    public int AdcRaw { get; set; }

    [JsonPropertyName("voltage")]
    public int Voltage { get; set; }

    [JsonPropertyName("vout")]
    public float Vout { get; set; }

    [JsonPropertyName("rs")]
    public float Rs { get; set; }

    [JsonPropertyName("ratio")]
    public float Ratio { get; set; }

    [JsonPropertyName("timestamp")]
    public DateTime Timestamp { get; set; } = DateTime.UtcNow;
}