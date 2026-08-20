<# : servefile.cmd
@echo off
setlocal

if "%~1"=="" (
    echo Usage: %~nx0 ^<file_to_serve^>
    exit /b 1
)

if not exist "%~1" (
    echo Error: File "%~1" does not exist.
    exit /b 1
)

powershell -NoProfile -ExecutionPolicy Bypass -Command "Invoke-Command -ScriptBlock ([ScriptBlock]::Create((Get-Content '%~f0' -Raw))) -ArgumentList '%~f1'"
exit /b %errorlevel%
#>

param([string]$FilePath)

$fullPath = (Resolve-Path $FilePath).Path
$name = [System.IO.Path]::GetFileName($fullPath)
$encodedName = [uri]::EscapeDataString($name)

# Get local IP address
$ip = (Get-NetIPAddress -AddressFamily IPv4 -InterfaceAlias Wi-Fi, Ethernet -ErrorAction SilentlyContinue | Where-Object IPAddress -notmatch '^169\.' | Select-Object -First 1).IPAddress
if (-not $ip) { $ip = [System.Net.Dns]::GetHostByName($env:COMPUTERNAME).AddressList[0].IPAddressToString }

# Passing port 0 tells Windows to automatically find and assign a free port
$listener = [System.Net.Sockets.TcpListener]::new([System.Net.IPAddress]::Any, 0)
$listener.Start()

# Retrieve the port number Windows actually gave us
$port = ($listener.LocalEndpoint -as [System.Net.IPEndPoint]).Port

Write-Host "Serving: $fullPath"
Write-Host "Network URL: http://${ip}:${port}/${encodedName}" -ForegroundColor Green
Write-Host "Waiting for connection... (Press Ctrl+C to cancel)"

while ($true) {
    # Accept incoming client connection
    $client = $listener.AcceptTcpClient()
    $stream = $client.GetStream()
    
    # Read client request header to parse the requested URL
    $reader = [System.IO.StreamReader]::new($stream, [System.Text.Encoding]::ASCII)
    $requestLine = $reader.ReadLine()

    if ($requestLine) {
        $parts = $requestLine.Split(' ')
        if ($parts.Length -ge 2) {
            $requestUrl = $parts[1].TrimStart('/')
            $decodedUrl = [uri]::UnescapeDataString($requestUrl)

            if ($decodedUrl -eq $name) {
                Write-Host "Connection from $($client.Client.RemoteEndPoint.Address). Sending file..." -ForegroundColor Cyan
                
                $fileStream = [System.IO.File]::OpenRead($fullPath)
                $fileLength = $fileStream.Length

                # Construct raw HTTP headers
                $header = "HTTP/1.1 200 OK`r`n" +
                          "Content-Type: application/octet-stream`r`n" +
                          "Content-Length: $fileLength`r`n" +
                          "Content-Disposition: attachment; filename=`"$name`"`r`n" +
                          "Connection: close`r`n`r`n"

                $headerBytes = [System.Text.Encoding]::ASCII.GetBytes($header)
                $stream.Write($headerBytes, 0, $headerBytes.Length)

                # Stream the file chunk by chunk to prevent RAM spikes
                $buffer = New-Object byte[] 65536
                while (($readCount = $fileStream.Read($buffer, 0, $buffer.Length)) -gt 0) {
                    $stream.Write($buffer, 0, $readCount)
                }

                $fileStream.Dispose()
                $stream.Flush()
                Write-Host "Transfer complete. Shutting down server." -ForegroundColor Green
                
                # Cleanup and break out of loop to exit script
                $client.Close()
                $listener.Stop()
                break
            } else {
                # Return a basic 404 response if they hit an incorrect path
                $responseString = "HTTP/1.1 404 Not Found`r`nConnection: close`r`n`r`n404 Not Found"
                $resBytes = [System.Text.Encoding]::ASCII.GetBytes($responseString)
                $stream.Write($resBytes, 0, $resBytes.Length)
                Write-Host "Ignored request for /$requestUrl" -ForegroundColor DarkGray
            }
        }
    }
    $client.Close()
}