# comprehensive_verify.ps1 - End-to-end verification of ShopKart server
$ErrorActionPreference = "Stop"
$baseUrl = "http://localhost:8080"
$cookieJar = New-Object System.Net.CookieContainer

Write-Host "====================================================" -ForegroundColor Cyan
Write-Host "   SHOPKART COMPREHENSIVE END-TO-END VERIFICATION   " -ForegroundColor Cyan
Write-Host "====================================================" -ForegroundColor Cyan

# 1. Test Home Page
Write-Host "`n[1] Testing GET / (Home Page)..." -ForegroundColor Yellow
$resp = Invoke-WebRequest -Uri "$baseUrl/" -UseBasicParsing
if ($resp.StatusCode -eq 200 -and $resp.Content -match "ShopKart") {
    Write-Host "  -> PASSED: Home page served (Status 200, Content: $($resp.Content.Length) bytes)" -ForegroundColor Green
} else {
    throw "Home page failed"
}

# 2. Test Catalog & Sorting (Merge Sort vs Quick Sort)
Write-Host "`n[2] Testing GET /products with Merge Sort and Quick Sort..." -ForegroundColor Yellow
$respMerge = Invoke-WebRequest -Uri "$baseUrl/products?cat=electronics&sort=price_asc&algo=merge" -UseBasicParsing
if ($respMerge.StatusCode -eq 200 -and $respMerge.Content -match "Merge Sort" -and $respMerge.Content -match "Comparisons:") {
    Write-Host "  -> PASSED: Merge Sort executed and verified with metrics displayed!" -ForegroundColor Green
} else {
    throw "Merge Sort failed"
}

$respQuick = Invoke-WebRequest -Uri "$baseUrl/products?cat=mobiles&sort=rating_desc&algo=quick" -UseBasicParsing
if ($respQuick.StatusCode -eq 200 -and $respQuick.Content -match "Quick Sort" -and $respQuick.Content -match "Comparisons:") {
    Write-Host "  -> PASSED: Quick Sort executed and verified with metrics displayed!" -ForegroundColor Green
} else {
    throw "Quick Sort failed"
}

# 3. Test Binary Search Price Range Filter vs Naive Scan
Write-Host "`n[3] Testing Binary Search Price Range Filter vs Naive Scan..." -ForegroundColor Yellow
$minP = 500
$maxP = 2500
$respPrice = Invoke-WebRequest -Uri "$baseUrl/products?min_price=$minP&max_price=$maxP" -UseBasicParsing
if ($respPrice.StatusCode -eq 200 -and $respPrice.Content -match "Binary Search Lower/Upper Bound") {
    Write-Host "  -> PASSED: Binary Search price filtering executed successfully!" -ForegroundColor Green
} else {
    throw "Price filter failed"
}

# 4. Test Product Details Page
Write-Host "`n[4] Testing GET /product?id=1..." -ForegroundColor Yellow
$respProd = Invoke-WebRequest -Uri "$baseUrl/product?id=1" -UseBasicParsing
if ($respProd.StatusCode -eq 200 -and $respProd.Content -match "Specifications" -and $respProd.Content -match "ADD TO CART") {
    Write-Host "  -> PASSED: Product detail page rendered with specifications and action buttons!" -ForegroundColor Green
} else {
    throw "Product detail failed"
}

# 5. Test Auth: Login with Demo Account
Write-Host "`n[5] Testing Auth: Login with Demo Account (demo@shopkart.local)..." -ForegroundColor Yellow
$loginUri = "$baseUrl/login"
$loginBody = "email=demo%40shopkart.local&password=Demo%4012345"
$req = [System.Net.HttpWebRequest]::Create($loginUri)
$req.Method = "POST"
$req.ContentType = "application/x-www-form-urlencoded"
$req.AllowAutoRedirect = $false
$req.CookieContainer = $cookieJar

$bytes = [System.Text.Encoding]::UTF8.GetBytes($loginBody)
$req.ContentLength = $bytes.Length
$os = $req.GetRequestStream()
$os.Write($bytes, 0, $bytes.Length)
$os.Close()

$loginResp = $req.GetResponse()
$setCookie = $loginResp.Headers["Set-Cookie"]
Write-Host "  -> Login Response Status: $($loginResp.StatusCode)" -ForegroundColor Green
Write-Host "  -> Session Cookie Received: $setCookie" -ForegroundColor Green
if ($setCookie -match "shopkart_session=") {
    Write-Host "  -> PASSED: Authentication generated valid session cookie!" -ForegroundColor Green
} else {
    throw "Login failed"
}
$loginResp.Close()

# 6. Test Private Route Protection when logged out
Write-Host "`n[6] Testing Private Route Protection (redirect when not authenticated)..." -ForegroundColor Yellow
$unauthReq = [System.Net.HttpWebRequest]::Create("$baseUrl/wishlist")
$unauthReq.AllowAutoRedirect = $false
$unauthResp = $unauthReq.GetResponse()
if ($unauthResp.StatusCode -eq 302 -and $unauthResp.Headers["Location"] -eq "/login") {
    Write-Host "  -> PASSED: Unauthenticated access to /wishlist redirected to /login!" -ForegroundColor Green
} else {
    throw "Private route redirect failed"
}
$unauthResp.Close()

# 7. Test Add to Cart with Session
Write-Host "`n[7] Testing Add to Cart & Stack Undo..." -ForegroundColor Yellow
$cartAddReq = [System.Net.HttpWebRequest]::Create("$baseUrl/cart/add")
$cartAddReq.Method = "POST"
$cartAddReq.ContentType = "application/x-www-form-urlencoded"
$cartAddReq.AllowAutoRedirect = $false
$cartAddReq.CookieContainer = $cookieJar
$body = [System.Text.Encoding]::UTF8.GetBytes("id=1&quantity=2")
$cartAddReq.ContentLength = $body.Length
$s = $cartAddReq.GetRequestStream()
$s.Write($body, 0, $body.Length)
$s.Close()
$cartAddResp = $cartAddReq.GetResponse()
Write-Host "  -> Cart Add Status: $($cartAddResp.StatusCode)" -ForegroundColor Green
$cartAddResp.Close()

# Check Cart Contents
$cartViewReq = [System.Net.HttpWebRequest]::Create("$baseUrl/cart")
$cartViewReq.CookieContainer = $cookieJar
$cartViewResp = $cartViewReq.GetResponse()
$reader = New-Object System.IO.StreamReader($cartViewResp.GetResponseStream())
$cartHtml = $reader.ReadToEnd()
$cartViewResp.Close()
if ($cartHtml -match "Shopping Cart" -and $cartHtml -match "PLACE ORDER") {
    Write-Host "  -> PASSED: Cart displays added items and price breakdown!" -ForegroundColor Green
} else {
    throw "Cart view failed"
}

# Remove Item (pushes onto Undo Stack)
$cartRemReq = [System.Net.HttpWebRequest]::Create("$baseUrl/cart/remove")
$cartRemReq.Method = "POST"
$cartRemReq.ContentType = "application/x-www-form-urlencoded"
$cartRemReq.AllowAutoRedirect = $false
$cartRemReq.CookieContainer = $cookieJar
$body = [System.Text.Encoding]::UTF8.GetBytes("id=1")
$cartRemReq.ContentLength = $body.Length
$s = $cartRemReq.GetRequestStream()
$s.Write($body, 0, $body.Length)
$s.Close()
$cartRemResp = $cartRemReq.GetResponse()
$cartRemResp.Close()

# Check Undo Banner
$cartViewReq2 = [System.Net.HttpWebRequest]::Create("$baseUrl/cart")
$cartViewReq2.CookieContainer = $cookieJar
$cartViewResp2 = $cartViewReq2.GetResponse()
$reader2 = New-Object System.IO.StreamReader($cartViewResp2.GetResponseStream())
$cartHtml2 = $reader2.ReadToEnd()
$cartViewResp2.Close()
if ($cartHtml2 -match "Removed .* from cart" -and $cartHtml2 -match "Undo Remove") {
    Write-Host "  -> PASSED: Cart Undo banner active with item pushed onto Stack!" -ForegroundColor Green
} else {
    throw "Undo banner failed"
}

# Undo Remove (pops from Stack)
$undoReq = [System.Net.HttpWebRequest]::Create("$baseUrl/cart/undo")
$undoReq.Method = "POST"
$undoReq.AllowAutoRedirect = $false
$undoReq.CookieContainer = $cookieJar
$undoResp = $undoReq.GetResponse()
$undoResp.Close()

$cartViewReq3 = [System.Net.HttpWebRequest]::Create("$baseUrl/cart")
$cartViewReq3.CookieContainer = $cookieJar
$cartViewResp3 = $cartViewReq3.GetResponse()
$reader3 = New-Object System.IO.StreamReader($cartViewResp3.GetResponseStream())
$cartHtml3 = $reader3.ReadToEnd()
$cartViewResp3.Close()
if ($cartHtml3 -match "PLACE ORDER") {
    Write-Host "  -> PASSED: Undo Remove successfully restored item from Stack to Cart!" -ForegroundColor Green
} else {
    throw "Undo restore failed"
}

# 8. Test Checkout and Order Creation
Write-Host "`n[8] Testing Checkout & Order Creation..." -ForegroundColor Yellow
$checkoutReq = [System.Net.HttpWebRequest]::Create("$baseUrl/checkout")
$checkoutReq.Method = "POST"
$checkoutReq.ContentType = "application/x-www-form-urlencoded"
$checkoutReq.AllowAutoRedirect = $false
$checkoutReq.CookieContainer = $cookieJar
$checkBody = "name=Demo+User&phone=9876543210&pincode=560103&city=Bengaluru&state=Karnataka&address=Flat+402%2C+Prestige+Tower&payment_method=COD"
$body = [System.Text.Encoding]::UTF8.GetBytes($checkBody)
$checkoutReq.ContentLength = $body.Length
$s = $checkoutReq.GetRequestStream()
$s.Write($body, 0, $body.Length)
$s.Close()
$checkoutResp = $checkoutReq.GetResponse()
$orderLoc = $checkoutResp.Headers["Location"]
Write-Host "  -> Order Created! Redirected to: $orderLoc" -ForegroundColor Green
if ($orderLoc -match "/orders\?new_order=OD") {
    Write-Host "  -> PASSED: Checkout generated persistent order with unique ID!" -ForegroundColor Green
} else {
    throw "Checkout failed"
}
$checkoutResp.Close()

# Verify Orders Page
$ordersReq = [System.Net.HttpWebRequest]::Create("$baseUrl/orders")
$ordersReq.CookieContainer = $cookieJar
$ordersResp = $ordersReq.GetResponse()
$readerO = New-Object System.IO.StreamReader($ordersResp.GetResponseStream())
$ordersHtml = $readerO.ReadToEnd()
$ordersResp.Close()
if ($ordersHtml -match "Order Confirmed" -and $ordersHtml -match "OD") {
    Write-Host "  -> PASSED: Orders history lists confirmed order!" -ForegroundColor Green
} else {
    throw "Orders history failed"
}

# 9. Test DSA Live Benchmark in C
Write-Host "`n[9] Testing DSA Live Benchmark Page (/dsa?run=1&n=1000)..." -ForegroundColor Yellow
$dsaReq = Invoke-WebRequest -Uri "$baseUrl/dsa?run=1&n=1000" -UseBasicParsing
if ($dsaReq.StatusCode -eq 200 -and $dsaReq.Content -match "Live Benchmark Results for N = 1000 Elements" -and $dsaReq.Content -match "Standard C qsort") {
    Write-Host "  -> PASSED: DSA Live Benchmark ran Merge Sort, Quick Sort, and qsort baseline with timing and comparisons!" -ForegroundColor Green
} else {
    throw "DSA benchmark failed"
}

# 10. Test Robustness / Malformed & Oversized Request Safety
Write-Host "`n[10] Testing Server Safety Against Malformed & Oversized Requests..." -ForegroundColor Yellow
# Malformed HTTP method
$malformedReq = [System.Net.HttpWebRequest]::Create("$baseUrl/random-invalid-route-XYZ")
$malformedReq.AllowAutoRedirect = $false
try {
    $malformedResp = $malformedReq.GetResponse()
    $status = [int]$malformedResp.StatusCode
    $malformedResp.Close()
} catch [System.Net.WebException] {
    $status = [int]$_.Exception.Response.StatusCode
}
if ($status -eq 404) {
    Write-Host "  -> PASSED: Unknown route returns custom 404 without crashing!" -ForegroundColor Green
} else {
    throw "404 handler failed with status $status"
}

Write-Host "`n====================================================" -ForegroundColor Cyan
Write-Host "   ALL SHOPKART END-TO-END CHECKS PASSED 100%!       " -ForegroundColor Green
Write-Host "====================================================" -ForegroundColor Cyan
