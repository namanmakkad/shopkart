#ifndef AUTH_H
#define AUTH_H

#include <time.h>
#include <stddef.h>

#define MAX_USER_NAME_LEN 64
#define MAX_EMAIL_LEN 128
#define MAX_HASH_LEN 65
#define MAX_SALT_LEN 33
#define SESSION_TOKEN_LEN 64
#define MAX_USERS 256
#define MAX_SESSIONS 512
#define MAX_FAILED_ATTEMPTS 5
#define LOCKOUT_DURATION_SEC 60
#define SESSION_EXPIRY_SEC (30 * 60)

typedef struct {
    char name[MAX_USER_NAME_LEN];
    char email[MAX_EMAIL_LEN];
    char salt[MAX_SALT_LEN];
    char password_hash[MAX_HASH_LEN];
    int failed_attempts;
    time_t lockout_until;
} User;

typedef struct {
    char token[SESSION_TOKEN_LEN + 1];
    char email[MAX_EMAIL_LEN];
    time_t created_at;
    time_t expires_at;
} Session;

/* Calculate SHA-256 of input buffer and format as 64-char lowercase hex string */
void sha256_hash_string(const char *input, char out_hex[MAX_HASH_LEN]);

/* Hash password combined with salt using SHA-256 */
void auth_hash_password(const char *password, const char *salt, char out_hash[MAX_HASH_LEN]);

/* Generate cryptographically secure random hexadecimal string */
void auth_generate_random_token(char *out_buf, size_t num_bytes);

/* Initialize auth subsystem and load users from CSV */
int auth_init(const char *users_csv_path);

/* Save current users back to CSV */
int auth_save_users(const char *users_csv_path);

/* Attempt login. Returns 1 on success, 0 on failure. Sets session token or error. */
int auth_login(const char *email, const char *password, char out_token[SESSION_TOKEN_LEN + 1], char out_error[128]);

/* Attempt signup. Returns 1 on success, 0 on failure. Sets session token or error. */
int auth_signup(const char *name, const char *email, const char *password, char out_token[SESSION_TOKEN_LEN + 1], char out_error[128]);

/* Validate session token and return user pointer, or NULL if invalid/expired */
const User *auth_get_user_by_session(const char *session_token);

/* Invalidate and delete a session */
void auth_logout(const char *session_token);

#endif /* AUTH_H */
