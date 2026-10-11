#include "auth.h"
#include "hashmap.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdint.h>

#ifdef _WIN32
  #include <windows.h>
  #include <wincrypt.h>
#else
  #include <fcntl.h>
  #include <unistd.h>
#endif

/* ================= SHA-256 IMPLEMENTATION ================= */

typedef struct {
    uint8_t data[64];
    uint32_t datalen;
    uint64_t bitlen;
    uint32_t state[8];
} SHA256_CTX;

#define ROTRIGHT(a,b) (((a) >> (b)) | ((a) << (32-(b))))
#define CH(x,y,z) (((x) & (y)) ^ (~(x) & (z)))
#define MAJ(x,y,z) (((x) & (y)) ^ ((x) & (z)) ^ ((y) & (z)))
#define EP0(x) (ROTRIGHT(x,2) ^ ROTRIGHT(x,13) ^ ROTRIGHT(x,22))
#define EP1(x) (ROTRIGHT(x,6) ^ ROTRIGHT(x,11) ^ ROTRIGHT(x,25))
#define SIG0(x) (ROTRIGHT(x,7) ^ ROTRIGHT(x,18) ^ ((x) >> 3))
#define SIG1(x) (ROTRIGHT(x,17) ^ ROTRIGHT(x,19) ^ ((x) >> 10))

static const uint32_t k_sha256[64] = {
    0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,
    0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,
    0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,
    0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,
    0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,
    0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
    0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,
    0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2
};

static void sha256_transform(SHA256_CTX *ctx, const uint8_t data[]) {
    uint32_t a, b, c, d, e, f, g, h, i, j, t1, t2, m[64];

    for (i = 0, j = 0; i < 16; ++i, j += 4)
        m[i] = ((uint32_t)data[j] << 24) | ((uint32_t)data[j + 1] << 16) | ((uint32_t)data[j + 2] << 8) | ((uint32_t)data[j + 3]);
    for ( ; i < 64; ++i)
        m[i] = SIG1(m[i - 2]) + m[i - 7] + SIG0(m[i - 15]) + m[i - 16];

    a = ctx->state[0];
    b = ctx->state[1];
    c = ctx->state[2];
    d = ctx->state[3];
    e = ctx->state[4];
    f = ctx->state[5];
    g = ctx->state[6];
    h = ctx->state[7];

    for (i = 0; i < 64; ++i) {
        t1 = h + EP1(e) + CH(e,f,g) + k_sha256[i] + m[i];
        t2 = EP0(a) + MAJ(a,b,c);
        h = g;
        g = f;
        f = e;
        e = d + t1;
        d = c;
        c = b;
        b = a;
        a = t1 + t2;
    }

    ctx->state[0] += a;
    ctx->state[1] += b;
    ctx->state[2] += c;
    ctx->state[3] += d;
    ctx->state[4] += e;
    ctx->state[5] += f;
    ctx->state[6] += g;
    ctx->state[7] += h;
}

static void sha256_init(SHA256_CTX *ctx) {
    ctx->datalen = 0;
    ctx->bitlen = 0;
    ctx->state[0] = 0x6a09e667;
    ctx->state[1] = 0xbb67ae85;
    ctx->state[2] = 0x3c6ef372;
    ctx->state[3] = 0xa54ff53a;
    ctx->state[4] = 0x510e527f;
    ctx->state[5] = 0x9b05688c;
    ctx->state[6] = 0x1f83d9ab;
    ctx->state[7] = 0x5be0cd19;
}

static void sha256_update(SHA256_CTX *ctx, const uint8_t data[], size_t len) {
    for (size_t i = 0; i < len; ++i) {
        ctx->data[ctx->datalen] = data[i];
        ctx->datalen++;
        if (ctx->datalen == 64) {
            sha256_transform(ctx, ctx->data);
            ctx->bitlen += 512;
            ctx->datalen = 0;
        }
    }
}

static void sha256_final(SHA256_CTX *ctx, uint8_t hash[]) {
    uint32_t i = ctx->datalen;

    if (ctx->datalen < 56) {
        ctx->data[i++] = 0x80;
        while (i < 56)
            ctx->data[i++] = 0x00;
    } else {
        ctx->data[i++] = 0x80;
        while (i < 64)
            ctx->data[i++] = 0x00;
        sha256_transform(ctx, ctx->data);
        memset(ctx->data, 0, 56);
    }

    ctx->bitlen += (uint64_t)ctx->datalen * 8;
    ctx->data[63] = (uint8_t)(ctx->bitlen);
    ctx->data[62] = (uint8_t)(ctx->bitlen >> 8);
    ctx->data[61] = (uint8_t)(ctx->bitlen >> 16);
    ctx->data[60] = (uint8_t)(ctx->bitlen >> 24);
    ctx->data[59] = (uint8_t)(ctx->bitlen >> 32);
    ctx->data[58] = (uint8_t)(ctx->bitlen >> 40);
    ctx->data[57] = (uint8_t)(ctx->bitlen >> 48);
    ctx->data[56] = (uint8_t)(ctx->bitlen >> 56);
    sha256_transform(ctx, ctx->data);

    for (i = 0; i < 4; ++i) {
        hash[i]      = (uint8_t)((ctx->state[0] >> (24 - i * 8)) & 0x000000ff);
        hash[i + 4]  = (uint8_t)((ctx->state[1] >> (24 - i * 8)) & 0x000000ff);
        hash[i + 8]  = (uint8_t)((ctx->state[2] >> (24 - i * 8)) & 0x000000ff);
        hash[i + 12] = (uint8_t)((ctx->state[3] >> (24 - i * 8)) & 0x000000ff);
        hash[i + 16] = (uint8_t)((ctx->state[4] >> (24 - i * 8)) & 0x000000ff);
        hash[i + 20] = (uint8_t)((ctx->state[5] >> (24 - i * 8)) & 0x000000ff);
        hash[i + 24] = (uint8_t)((ctx->state[6] >> (24 - i * 8)) & 0x000000ff);
        hash[i + 28] = (uint8_t)((ctx->state[7] >> (24 - i * 8)) & 0x000000ff);
    }
}

void sha256_hash_string(const char *input, char out_hex[MAX_HASH_LEN]) {
    if (!input || !out_hex) return;

    SHA256_CTX ctx;
    uint8_t hash[32];
    sha256_init(&ctx);
    sha256_update(&ctx, (const uint8_t *)input, strlen(input));
    sha256_final(&ctx, hash);

    for (int i = 0; i < 32; i++) {
        snprintf(out_hex + (i * 2), 3, "%02x", hash[i]);
    }
    out_hex[64] = '\0';
}

void auth_hash_password(const char *password, const char *salt, char out_hash[MAX_HASH_LEN]) {
    char combined[512];
    snprintf(combined, sizeof(combined), "%s:%s", salt ? salt : "", password ? password : "");
    sha256_hash_string(combined, out_hash);
}

void auth_generate_random_token(char *out_buf, size_t num_bytes) {
    if (!out_buf || num_bytes == 0) return;

    uint8_t random_bytes[32];
    size_t count = (num_bytes < sizeof(random_bytes)) ? num_bytes : sizeof(random_bytes);

    int acquired = 0;
#ifdef _WIN32
    HCRYPTPROV hCryptProv;
    if (CryptAcquireContext(&hCryptProv, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT)) {
        if (CryptGenRandom(hCryptProv, (DWORD)count, random_bytes)) {
            acquired = 1;
        }
        CryptReleaseContext(hCryptProv, 0);
    }
#else
    int fd = open("/dev/urandom", O_RDONLY);
    if (fd >= 0) {
        if (read(fd, random_bytes, count) == (ssize_t)count) {
            acquired = 1;
        }
        close(fd);
    }
#endif

    if (!acquired) {
        /* Pseudo-random fallback */
        srand((unsigned int)time(NULL) ^ (unsigned int)(uintptr_t)out_buf);
        for (size_t i = 0; i < count; i++) {
            random_bytes[i] = (uint8_t)(rand() % 256);
        }
    }

    for (size_t i = 0; i < count; i++) {
        snprintf(out_buf + (i * 2), 3, "%02x", random_bytes[i]);
    }
    out_buf[count * 2] = '\0';
}

/* ================= AUTH & SESSION STATE ================= */

static User g_users[MAX_USERS];
static int g_user_count = 0;
static HashMap g_user_map;     /* email -> User* */

static Session g_sessions[MAX_SESSIONS];
static int g_session_count = 0;
static HashMap g_session_map;  /* token -> Session* */
static char g_users_file[256];

int auth_init(const char *users_csv_path) {
    if (users_csv_path) {
        snprintf(g_users_file, sizeof(g_users_file), "%s", users_csv_path);
    } else {
        snprintf(g_users_file, sizeof(g_users_file), "data/users.csv");
    }

    hashmap_init(&g_user_map);
    hashmap_init(&g_session_map);
    g_user_count = 0;
    g_session_count = 0;

    FILE *fp = fopen(g_users_file, "r");
    if (fp) {
        char line[512];
        /* Skip header if present */
        if (fgets(line, sizeof(line), fp)) {
            if (strstr(line, "email") == NULL) {
                /* Not a header line, process it */
                fseek(fp, 0, SEEK_SET);
            }
        }

        while (fgets(line, sizeof(line), fp) && g_user_count < MAX_USERS) {
            size_t len = strlen(line);
            while (len > 0 && (line[len - 1] == '\r' || line[len - 1] == '\n')) {
                line[--len] = '\0';
            }
            if (len == 0) continue;

            User *u = &g_users[g_user_count];
            memset(u, 0, sizeof(User));

            char f_name[MAX_USER_NAME_LEN] = {0};
            char f_email[MAX_EMAIL_LEN] = {0};
            char f_salt[MAX_SALT_LEN] = {0};
            char f_hash[MAX_HASH_LEN] = {0};

            /* Format: name,email,salt,password_hash */
            int parsed = sscanf(line, "%63[^,],%127[^,],%32[^,],%64s", f_name, f_email, f_salt, f_hash);
            if (parsed == 4) {
                snprintf(u->name, sizeof(u->name), "%s", f_name);
                snprintf(u->email, sizeof(u->email), "%s", f_email);
                snprintf(u->salt, sizeof(u->salt), "%s", f_salt);
                snprintf(u->password_hash, sizeof(u->password_hash), "%s", f_hash);
                u->failed_attempts = 0;
                u->lockout_until = 0;

                hashmap_put(&g_user_map, u->email, u->email, u);
                g_user_count++;
            }
        }
        fclose(fp);
    }

    /* Seed Demo User if not present */
    if (!hashmap_get_data(&g_user_map, "demo@shopkart.local") && g_user_count < MAX_USERS) {
        User *demo = &g_users[g_user_count++];
        memset(demo, 0, sizeof(User));
        snprintf(demo->name, sizeof(demo->name), "Demo User");
        snprintf(demo->email, sizeof(demo->email), "demo@shopkart.local");
        snprintf(demo->salt, sizeof(demo->salt), "shopkart_salt_demo_2024");
        auth_hash_password("Demo@12345", demo->salt, demo->password_hash);
        demo->failed_attempts = 0;
        demo->lockout_until = 0;

        hashmap_put(&g_user_map, demo->email, demo->email, demo);
        auth_save_users(g_users_file);
    }

    return g_user_count;
}

int auth_save_users(const char *users_csv_path) {
    const char *path = users_csv_path ? users_csv_path : g_users_file;
    FILE *fp = fopen(path, "w");
    if (!fp) return 0;

    fprintf(fp, "name,email,salt,password_hash\n");
    for (int i = 0; i < g_user_count; i++) {
        fprintf(fp, "%s,%s,%s,%s\n",
                g_users[i].name,
                g_users[i].email,
                g_users[i].salt,
                g_users[i].password_hash);
    }
    fclose(fp);
    return 1;
}

int auth_login(const char *email, const char *password, char out_token[SESSION_TOKEN_LEN + 1], char out_error[128]) {
    if (out_error) out_error[0] = '\0';
    if (out_token) out_token[0] = '\0';

    if (!email || !password || email[0] == '\0' || password[0] == '\0') {
        if (out_error) snprintf(out_error, 128, "Email and password are required.");
        return 0;
    }

    User *user = (User *)hashmap_get_data(&g_user_map, email);
    if (!user) {
        if (out_error) snprintf(out_error, 128, "Invalid email or password.");
        return 0;
    }

    time_t now = time(NULL);
    if (user->lockout_until > now) {
        int remaining = (int)(user->lockout_until - now);
        if (out_error) snprintf(out_error, 128, "Account temporarily locked. Please try again in %d seconds.", remaining);
        return 0;
    }

    char computed_hash[MAX_HASH_LEN];
    auth_hash_password(password, user->salt, computed_hash);

    if (strcmp(computed_hash, user->password_hash) != 0) {
        user->failed_attempts++;
        if (user->failed_attempts >= MAX_FAILED_ATTEMPTS) {
            user->lockout_until = now + LOCKOUT_DURATION_SEC;
            user->failed_attempts = 0;
            if (out_error) snprintf(out_error, 128, "Too many failed attempts. Account locked for 60 seconds.");
        } else {
            int left = MAX_FAILED_ATTEMPTS - user->failed_attempts;
            if (out_error) snprintf(out_error, 128, "Invalid email or password. (%d attempts remaining)", left);
        }
        return 0;
    }

    /* Reset lockout counters on success */
    user->failed_attempts = 0;
    user->lockout_until = 0;

    /* Create Session */
    char token[SESSION_TOKEN_LEN + 1];
    auth_generate_random_token(token, 16); /* 32 hex chars */

    Session *sess = NULL;
    if (g_session_count < MAX_SESSIONS) {
        sess = &g_sessions[g_session_count++];
    } else {
        /* Reuse oldest session slot */
        sess = &g_sessions[0];
        hashmap_remove(&g_session_map, sess->token);
    }

    snprintf(sess->token, sizeof(sess->token), "%s", token);
    snprintf(sess->email, sizeof(sess->email), "%s", user->email);
    sess->created_at = now;
    sess->expires_at = now + SESSION_EXPIRY_SEC;

    hashmap_put(&g_session_map, sess->token, sess->email, sess);

    if (out_token) {
        snprintf(out_token, SESSION_TOKEN_LEN + 1, "%s", token);
    }
    return 1;
}

int auth_signup(const char *name, const char *email, const char *password, char out_token[SESSION_TOKEN_LEN + 1], char out_error[128]) {
    if (out_error) out_error[0] = '\0';
    if (out_token) out_token[0] = '\0';

    if (!name || !email || !password || name[0] == '\0' || email[0] == '\0' || password[0] == '\0') {
        if (out_error) snprintf(out_error, 128, "All fields are required.");
        return 0;
    }

    if (strlen(password) < 6) {
        if (out_error) snprintf(out_error, 128, "Password must be at least 6 characters.");
        return 0;
    }

    if (hashmap_get_data(&g_user_map, email) != NULL) {
        if (out_error) snprintf(out_error, 128, "An account with this email already exists.");
        return 0;
    }

    if (g_user_count >= MAX_USERS) {
        if (out_error) snprintf(out_error, 128, "User database capacity reached.");
        return 0;
    }

    User *user = &g_users[g_user_count++];
    memset(user, 0, sizeof(User));
    snprintf(user->name, sizeof(user->name), "%s", name);
    snprintf(user->email, sizeof(user->email), "%s", email);

    char salt[MAX_SALT_LEN];
    auth_generate_random_token(salt, 8);
    snprintf(user->salt, sizeof(user->salt), "%s", salt);

    auth_hash_password(password, user->salt, user->password_hash);
    user->failed_attempts = 0;
    user->lockout_until = 0;

    hashmap_put(&g_user_map, user->email, user->email, user);
    auth_save_users(g_users_file);

    return auth_login(email, password, out_token, out_error);
}

const User *auth_get_user_by_session(const char *session_token) {
    if (!session_token || session_token[0] == '\0') return NULL;

    Session *sess = (Session *)hashmap_get_data(&g_session_map, session_token);
    if (!sess) return NULL;

    time_t now = time(NULL);
    if (sess->expires_at < now) {
        hashmap_remove(&g_session_map, session_token);
        return NULL;
    }

    return (const User *)hashmap_get_data(&g_user_map, sess->email);
}

void auth_logout(const char *session_token) {
    if (!session_token || session_token[0] == '\0') return;
    hashmap_remove(&g_session_map, session_token);
}
