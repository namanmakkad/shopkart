# Script to generate rich product dataset
$categories = @(
    @{
        name = "electronics"
        sub = @("headphones", "speakers", "cameras", "laptops", "tablets")
        brands = @("Sony", "boAt", "JBL", "Apple", "Dell", "HP", "Canon", "Bose")
        titles = @(
            "Noise Cancelling Over-Ear Wireless Headphones",
            "Portable Bluetooth Speaker with Deep Bass",
            "Mirrorless 4K Vlogging Digital Camera",
            "Ultra-Thin Intel Core i7 Laptop 16GB RAM",
            "10.9-inch Retina Display Wi-Fi Tablet",
            "True Wireless Gaming Earbuds Low Latency",
            "Surround Sound Home Theatre Soundbar",
            "Full Frame Professional Cinema DSLR Camera",
            "Gaming Laptop RTX 4060 144Hz FHD",
            "Pro Stylus Compatible Productivity Tablet",
            "High-Resolution Studio Monitor Headphones",
            "Waterproof Party Speaker with Dynamic LED",
            "Compact Travel Zoom Optical Camera"
        )
    },
    @{
        name = "mobiles"
        sub = @("smartphones", "budget", "cases", "chargers", "smartwatches")
        brands = @("Samsung", "Apple", "OnePlus", "Xiaomi", "Realme", "Google", "Motorola")
        titles = @(
            "Galaxy Flagship 5G AMOLED 120Hz Display",
            "Super Retina OLED A16 Bionic Smartphone",
            "Nord 5G Warp Charge 8GB 128GB Storage",
            "Redmi Note Pro Max 108MP Quad Camera",
            "Narzo Fast Performance 5000mAh Battery",
            "Pixel Tensor AI Pure Android Camera Phone",
            "Edge Ultra 5G Snapdragon Smooth Display",
            "Rugged Shockproof Armor Protective Case",
            "65W GaN Dual Port USB-C Super Fast Charger",
            "AMOLED Bluetooth Calling Smartwatch with SpO2",
            "MagSafe Wireless Magnetic Battery Pack",
            "Curved Screen Gorilla Glass Budget Smartphone",
            "Fitness Tracker Water Resistant Smart Band"
        )
    },
    @{
        name = "fashion"
        sub = @("mens-shirts", "mens-shoes", "womens-dresses", "womens-shoes", "watches")
        brands = @("Nike", "Puma", "Levi's", "Zara", "Adidas", "Allen Solly", "Titan")
        titles = @(
            "Slim Fit 100% Breathable Cotton Casual Shirt",
            "Air Cushion Lightweight Running Sneakers",
            "Floral Printed Summer Casual Midi Dress",
            "Comfort Cushion Memory Foam Walking Flats",
            "Chronograph Analogue Quartz Stainless Watch",
            "Classic Indigo Denim Washed Jeans Straight Fit",
            "Athletic Training Lightweight Running Shoes",
            "A-Line Bohemian Party Maxi Designer Dress",
            "Formal Derby Genuine Leather Office Shoes",
            "Minimalist Rose Gold Slim Women's Watch",
            "Formal Checked Cotton Long Sleeve Shirt",
            "Everyday Canvas Low-Top Casual Sneakers",
            "Silk Blend Embroidered Festive Ethnic Kurti"
        )
    },
    @{
        name = "home"
        sub = @("sofas", "tables", "cookware", "storage", "lighting")
        brands = @("Ikea", "Sleepwell", "Prestige", "Milton", "Philips", "HomeTown")
        titles = @(
            "Modern 3-Seater Fabric Recliner Sofa",
            "Engineered Wood Solid Work-from-Home Desk",
            "Hard Anodized Non-Stick Tri-Ply Cookware Set",
            "Multi-Tier Modular Plastic Storage Drawer",
            "Smart Wi-Fi RGB LED Ambient Floor Lamp",
            "Orthopedic High Resilience Foam Queen Mattress",
            "Solid Sheesham Wood 4-Seater Dining Table",
            "Heavy Stainless Steel Pressure Cooker 5L",
            "Vacuum Insulated Stainless Steel Water Bottle",
            "Adjustable Desk Lamp with Warm Dimming",
            "L-Shape Corner Sectional Luxury Sofa",
            "Foldable Laptop Bed Tray with Cup Holder",
            "Glass Air-Tight Kitchen Container Combo"
        )
    },
    @{
        name = "appliances"
        sub = @("refrigerators", "washing", "mixers", "irons", "microwaves")
        brands = @("LG", "Samsung", "Whirlpool", "Bajaj", "IFB", "Philips", "Morphy")
        titles = @(
            "Double Door Frost Free Smart Inverter Fridge",
            "Front Load Fully Automatic 8kg Washing Machine",
            "750W Heavy Duty Mixer Grinder 3 Jars",
            "Non-Stick Soleplate Steam Iron 1200W",
            "Convection 28L Microwave Oven with Auto Cook",
            "Side by Side Multi-Airflow Inverter Refrigerator",
            "Top Load Quick Wash 7kg Washing Machine",
            "Powerful Nutri-Bullet Smoothie Blender",
            "Cordless Ceramic Coating Garment Steamer",
            "Touch Screen Digital OTG Grill Oven 30L",
            "Single Door Direct Cool Energy Saver Fridge",
            "Semi-Automatic Turbo Dry Washing Machine",
            "Silent Operation Powerful Food Processor"
        )
    },
    @{
        name = "food"
        sub = @("chips", "chocolates", "tea", "juices", "staples")
        brands = @("Lays", "Cadbury", "Tata", "Real", "Kellogg's", "Nestle", "Amul")
        titles = @(
            "Spanish Tomato Crunch Potato Chips Party Pack",
            "Silk Roasted Almond Melt-in-Mouth Chocolate",
            "Premium Gold Assam Kadak Leaf Black Tea 1kg",
            "100% Pure Mixed Fruit Healthy Juice 1L",
            "Whole Grain Corn Flakes with Real Almonds",
            "Classic Classic Salted Crispy Potato Wafers",
            "Dark Chocolate 70% Cocoa Intense Rich Bar",
            "Darjeeling Green Tea with Pure Lemon & Honey",
            "Pomegranate Nutrient Rich Fresh Fruit Juice",
            "Crunchy Nut Granola Muesli High Fibre 500g",
            "Rich Pure Cow Ghee Traditional Aroma 1L",
            "Instant Roast Hazelnut Coffee Beans Powder",
            "Organic Basmati Long Grain Aromatic Rice 5kg"
        )
    },
    @{
        name = "beauty"
        sub = @("moisturizers", "sunscreen", "shampoo", "fragrances", "serums")
        brands = @("Nivea", "L'Oreal", "Neutrogena", "Minimalist", "Plum", "Dove")
        titles = @(
            "Ultra Light Gel Oil-Free Face Moisturizer",
            "Matte Ultra Sheer Sunscreen SPF 50+ PA++++",
            "Total Repair 5 Keratin Protein Shampoo 650ml",
            "Eau De Parfum Long Lasting Luxury Fragrance",
            "10% Niacinamide + Zinc Glowing Face Serum",
            "Deep Nourishing Shea Butter Body Lotion",
            "Water Gel Hydrating Hyaluronic Face Cream",
            "Anti-Hairfall Biotin Strengthening Conditioner",
            "Gentle Foaming Tea Tree Oil Face Cleanser",
            "Vitamin C Brightening Dark Spot Correcting Serum",
            "Natural Charcoal Detox Deep Pore Clay Mask",
            "Argan Oil Intense Frizz Control Hair Serum",
            "Aloe Vera Soothing Multipurpose Organic Gel"
        )
    },
    @{
        name = "toys"
        sub = @("action-figures", "board-games", "puzzles", "remote-cars", "lego")
        brands = @("Lego", "Hasbro", "Mattel", "Hot Wheels", "Funskool", "Nerf")
        titles = @(
            "City Police Station Interactive Building Blocks",
            "Classic Fast-Dealing Property Trading Board Game",
            "1000-Piece Panoramic World Landmark Jigsaw Puzzle",
            "High Speed RC Monster Racing Buggy 4WD",
            "Elite Rapid Fire Dart Blaster with 20 Darts",
            "Superhero Avengers Movable Armor Action Figure",
            "Strategy Mystery Detective Clue Board Game",
            "3D Wooden Mechanical Solar System Puzzle",
            "Die-Cast 1:64 Scale Metal Track 5-Car Pack",
            "Creative Brick Box 500 Pieces Endless Fun",
            "Electronic Interactive Talking Robot Toy",
            "Speed Cube 3x3 Ultra Smooth Turn Puzzle",
            "All-Terrain Stunt Flip Remote Control Car"
        )
    },
    @{
        name = "sports"
        sub = @("cricket", "badminton", "fitness", "footballs", "yoga")
        brands = @("Yonex", "Cosco", "Decathlon", "Nivia", "SG", "Strauss")
        titles = @(
            "English Willow Kashmir Grade Cricket Bat",
            "Carbon Fiber Lightweight Badminton Racket",
            "Anti-Slip High Density 6mm Eco Yoga Mat",
            "FIFA Quality Match TPU Training Football Size 5",
            "Adjustable Dumbbell Set with Chrome Bar 20kg",
            "Professional Leather Cricket Ball Box of 4",
            "Nylon Shuttlecock Aero Speed Pack of 6",
            "Resistance Loop Bands Set of 5 Tension Levels",
            "Professional Goalkeeper Gloves with Finger Grip",
            "Skipping Jump Rope High Speed Ball Bearings",
            "Cricket Batting Protective Padded Gloves",
            "Foam Muscle Deep Tissue Massage Roller",
            "Heavy Punching Bag with Hanging Chain & Gloves"
        )
    },
    @{
        name = "books"
        sub = @("fiction", "non-fiction", "self-help", "tech", "children")
        brands = @("Penguin", "HarperCollins", "Rupa", "O'Reilly", "Scholastic")
        titles = @(
            "The Midnight Library Best-Selling Hardcover",
            "Atomic Habits Easy Proven Way to Build Good Habits",
            "Sapiens A Brief History of Humankind",
            "Clean Code Agile Software Craftsmanship Guide",
            "Harry Potter Illustrated Collectors Edition",
            "The Psychology of Money Timeless Lessons",
            "Deep Work Rules for Focused Success",
            "Introduction to Algorithms 4th Edition Hardcover",
            "Ikigai Japanese Secret to Long and Happy Life",
            "Rich Dad Poor Dad What the Rich Teach Kids",
            "The Alchemist Modern Classic Philosophy Novel",
            "Designing Data-Intensive Applications Standard",
            "The Very Hungry Caterpillar Classic Board Book"
        )
    }
)

$id = 1
$lines = @("id,name,brand,category,subcategory,price,mrp,rating,review_count,stock,image,highlights,specs")

foreach ($cat in $categories) {
    for ($i = 0; $i -lt 13; $i++) {
        $sub = $cat.sub[$i % $cat.sub.Count]
        $brand = $cat.brands[$i % $cat.brands.Count]
        $title = $brand + " " + $cat.titles[$i]
        
        $basePrice = 300 + (($id * 277) % 35000)
        $mrp = [Math]::Max(499, $basePrice + (($id * 149) % 8000))
        $discountPct = 15 + (($id * 7) % 55)
        $price = [Math]::Max(199, [int]($mrp * (100 - $discountPct) / 100))
        $rating = [string]::Format("{0:0.0}", 3.6 + (($id * 7) % 14) / 10.0)
        $reviews = 40 + (($id * 183) % 12000)
        $stock = 8 + (($id * 13) % 90)
        $img = "cat-" + $cat.name + "-" + $sub
        $hl = "Genuine " + $brand + " Product | 1 Year Replacement Warranty | 7 Days Return Policy | Cash on Delivery Available"
        $sp = "Brand: " + $brand + " | Category: " + $cat.name + " | Subcategory: " + $sub + " | Model ID: SK-" + $id + " | Rating: " + $rating + " Stars | In Stock: " + $stock + " units"

        # Escape quotes in fields
        $escapedTitle = $title.Replace('"', '""')
        $escapedHl = $hl.Replace('"', '""')
        $escapedSp = $sp.Replace('"', '""')

        $line = "$id,`"$escapedTitle`",$brand,$($cat.name),$sub,$price,$mrp,$rating,$reviews,$stock,$img,`"$escapedHl`",`"$escapedSp`""
        $lines += $line
        $id++
    }
}

$outputFile = "data/products.csv"
[System.IO.File]::WriteAllLines($outputFile, $lines, [System.Text.Encoding]::UTF8)
Write-Host "Generated $($id - 1) products in $outputFile"
