$BaseUrl = "http://127.0.0.1:5000"

Write-Host "=== Testing DistributedCache++ API at $BaseUrl ==="

# --- PUT ---
Write-Host "PUT foo=bar"
$putRes = Invoke-RestMethod -Method PUT -Uri "$BaseUrl/cache/foo" `
    -ContentType "application/json" `
    -Body '{"value":"bar","ttl":60}'

if ($putRes.status -ne "ok") { throw "❌ PUT failed" }

# --- GET ---
Write-Host "GET foo"
$getRes = Invoke-RestMethod -Uri "$BaseUrl/cache/foo"
if ($getRes.value -ne "bar") { throw "❌ GET failed" }

# --- DELETE ---
Write-Host "DELETE foo"
$delRes = Invoke-RestMethod -Method DELETE -Uri "$BaseUrl/cache/foo"
if ($delRes.status -ne "deleted") { throw "❌ DELETE failed" }

# --- GET again (expect 404) ---
Write-Host "GET foo (expect 404)"
try {
    Invoke-RestMethod -Uri "$BaseUrl/cache/foo" -ErrorAction Stop
    throw "❌ Expected 404 but key was found"
} catch [System.Net.WebException] {
    if ($_.Exception.Response.StatusCode.value__ -ne 404) {
        throw "❌ Expected 404, got different error"
    }
}

# --- Metrics ---
Write-Host "GET /metrics"
$metrics = Invoke-RestMethod -Uri "$BaseUrl/metrics"
if ($metrics -notmatch "cache_hits_total") { throw "❌ Metrics missing" }

Write-Host "✅ All API tests passed!"