using System;
using System.IO;
using System.Net;
using System.Security.Principal;
using System.Text;
using System.Text.Json;
using System.Threading;
using System.Threading.Tasks;

class Program
{
    private static FanController? _controller;
    private static HttpListener? _listener;
    private static bool _running = true;

    static void Main(string[] args)
    {
        // Check Administrator privileges
        if (!IsAdministrator())
        {
            Console.Error.WriteLine("Error: LibreHardwareMonitor requires Administrator privileges to access hardware sensors.");
            Environment.Exit(1);
        }

        // Handle stdin monitoring for clean exit when parent process closes
        StartStdinMonitor();

        try
        {
            _controller = new FanController();
        }
        catch (Exception ex)
        {
            Console.Error.WriteLine($"Failed to initialize FanController: {ex.Message}");
            Environment.Exit(2);
        }

        // Register process exit handler
        AppDomain.CurrentDomain.ProcessExit += (s, e) => CleanUp();

        // Start HTTP Server
        _listener = new HttpListener();
        _listener.Prefixes.Add("http://localhost:5555/");
        try
        {
            _listener.Start();
            Console.WriteLine("Server started on http://localhost:5555/");
        }
        catch (Exception ex)
        {
            Console.Error.WriteLine($"Failed to start HTTP listener: {ex.Message}");
            CleanUp();
            Environment.Exit(3);
        }

        // Request handling loop
        Task.Run(async () =>
        {
            while (_running)
            {
                try
                {
                    var context = await _listener.GetContextAsync();
                    _ = Task.Run(() => HandleRequest(context));
                }
                catch (Exception)
                {
                    // Listener might have stopped
                }
            }
        });

        // Keep main thread alive
        while (_running)
        {
            Thread.Sleep(100);
        }
    }

    private static bool IsAdministrator()
    {
        using (var identity = WindowsIdentity.GetCurrent())
        {
            var principal = new WindowsPrincipal(identity);
            return principal.IsInRole(WindowsBuiltInRole.Administrator);
        }
    }

    private static void StartStdinMonitor()
    {
        var thread = new Thread(() =>
        {
            try
            {
                // ReadLine blocks until stdin is closed, returning null
                while (Console.ReadLine() != null) { }
            }
            catch { }
            Console.WriteLine("Stdin closed, exiting backend...");
            _running = false;
            CleanUp();
            Environment.Exit(0);
        });
        thread.IsBackground = true;
        thread.Start();
    }

    private static void CleanUp()
    {
        _running = false;
        try
        {
            _listener?.Stop();
        }
        catch { }
        try
        {
            _controller?.Dispose();
        }
        catch { }
    }

    private static async Task HandleRequest(HttpListenerContext context)
    {
        var request = context.Request;
        var response = context.Response;

        // Set response headers
        response.Headers.Add("Access-Control-Allow-Origin", "*");
        response.Headers.Add("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
        response.Headers.Add("Access-Control-Allow-Headers", "Content-Type");
        response.ContentType = "application/json";

        if (request.HttpMethod == "OPTIONS")
        {
            response.StatusCode = (int)HttpStatusCode.OK;
            response.Close();
            return;
        }

        string path = request.Url?.AbsolutePath ?? "";
        string method = request.HttpMethod;

        try
        {
            if (method == "GET" && path == "/fans")
            {
                var fans = _controller?.GetFans() ?? new();
                string json = JsonSerializer.Serialize(fans);
                await WriteResponse(response, json, HttpStatusCode.OK);
            }
            else if (method == "GET" && path == "/controls")
            {
                var controls = _controller?.GetControls() ?? new();
                string json = JsonSerializer.Serialize(controls);
                await WriteResponse(response, json, HttpStatusCode.OK);
            }
            else if (method == "POST" && path.StartsWith("/controls/"))
            {
                string relativePath = path.Substring("/controls/".Length);
                
                if (relativePath.EndsWith("/auto"))
                {
                    // POST /controls/:id/auto
                    string id = relativePath.Substring(0, relativePath.Length - "/auto".Length);
                    bool success = _controller?.SetControlAuto(id) ?? false;
                    
                    if (success)
                    {
                        await WriteResponse(response, "{\"status\":\"success\"}", HttpStatusCode.OK);
                    }
                    else
                    {
                        await WriteResponse(response, "{\"error\":\"Control ID not found or not controllable\"}", HttpStatusCode.NotFound);
                    }
                }
                else
                {
                    // POST /controls/:id
                    string id = relativePath;
                    using var reader = new StreamReader(request.InputStream, request.ContentEncoding ?? Encoding.UTF8);
                    string body = await reader.ReadToEndAsync();
                    
                    try
                    {
                        using var doc = JsonDocument.Parse(body);
                        var root = doc.RootElement;
                        
                        string mode = root.TryGetProperty("mode", out var modeProp) ? (modeProp.GetString() ?? "") : "";
                        
                        if (mode.ToLower() == "manual")
                        {
                            float speed = root.TryGetProperty("speed", out var speedProp) ? (float)speedProp.GetDouble() : 0.0f;
                            bool resetOnExit = !root.TryGetProperty("resetOnExit", out var resetProp) || resetProp.GetBoolean();
                            bool success = _controller?.SetControlManual(id, speed, resetOnExit) ?? false;
                            
                            if (success)
                            {
                                await WriteResponse(response, "{\"status\":\"success\"}", HttpStatusCode.OK);
                            }
                            else
                            {
                                await WriteResponse(response, "{\"error\":\"Control ID not found or not controllable\"}", HttpStatusCode.NotFound);
                            }
                        }
                        else if (mode.ToLower() == "auto")
                        {
                            bool success = _controller?.SetControlAuto(id) ?? false;
                            if (success)
                            {
                                await WriteResponse(response, "{\"status\":\"success\"}", HttpStatusCode.OK);
                            }
                            else
                            {
                                await WriteResponse(response, "{\"error\":\"Control ID not found\"}", HttpStatusCode.NotFound);
                            }
                        }
                        else
                        {
                            await WriteResponse(response, "{\"error\":\"Invalid mode. Use manual or auto.\"}", HttpStatusCode.BadRequest);
                        }
                    }
                    catch (Exception ex)
                    {
                        await WriteResponse(response, $"{{\"error\":\"Invalid JSON body: {ex.Message}\"}}", HttpStatusCode.BadRequest);
                    }
                }
            }
            else
            {
                await WriteResponse(response, "{\"error\":\"Not Found\"}", HttpStatusCode.NotFound);
            }
        }
        catch (Exception ex)
        {
            await WriteResponse(response, $"{{\"error\":\"Internal Server Error: {ex.Message}\"}}", HttpStatusCode.InternalServerError);
        }
    }

    private static async Task WriteResponse(HttpListenerResponse response, string content, HttpStatusCode status)
    {
        response.StatusCode = (int)status;
        byte[] buffer = Encoding.UTF8.GetBytes(content);
        response.ContentLength64 = buffer.Length;
        try
        {
            await response.OutputStream.WriteAsync(buffer, 0, buffer.Length);
            response.OutputStream.Close();
        }
        catch { }
    }
}
