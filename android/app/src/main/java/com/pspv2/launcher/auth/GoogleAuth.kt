package com.pspv2.launcher.auth

import android.content.Context
import android.util.Log
import androidx.credentials.CredentialManager
import androidx.credentials.GetCredentialRequest
import com.google.android.libraries.identity.googleid.GetGoogleIdOption
import com.google.android.libraries.identity.googleid.GoogleIdTokenCredential

/**
 * Minimal "Sign in with Google" helper built on Credential Manager (the modern
 * replacement for the deprecated GoogleSignIn API). It only captures the account's
 * display name, email and stable id — no Drive/cloud scopes — so the player's PSPV2
 * profile can be tied to their Google account.
 *
 * To enable it, paste your OAuth **Web application** client ID into [WEB_CLIENT_ID]
 * (Google Cloud Console → Credentials). Until then [isConfigured] is false and the
 * UI falls back to a locally typed username.
 */
object GoogleAuth {

    private const val TAG = "GoogleAuth"

    /**
     * OAuth 2.0 **Web application** client ID from the Google Cloud Console.
     * Looks like: 1234567890-abcdefg.apps.googleusercontent.com
     */
    const val WEB_CLIENT_ID: String = ""

    /** Result of a sign-in attempt. */
    data class Account(val id: String, val displayName: String, val email: String)

    /** True once a real Web client ID has been configured. */
    val isConfigured: Boolean
        get() = WEB_CLIENT_ID.isNotBlank() && WEB_CLIENT_ID.endsWith(".apps.googleusercontent.com")

    /**
     * Launches the Google account chooser and returns the chosen account, or null if
     * sign-in was cancelled, failed, or hasn't been configured yet. Must be called
     * from a coroutine; it suspends while the system UI is shown.
     */
    suspend fun signIn(context: Context): Account? {
        if (!isConfigured) {
            Log.w(TAG, "GoogleAuth.WEB_CLIENT_ID not set; skipping Google sign-in")
            return null
        }
        return try {
            val option = GetGoogleIdOption.Builder()
                .setServerClientId(WEB_CLIENT_ID)
                .setFilterByAuthorizedAccounts(false)
                .setAutoSelectEnabled(false)
                .build()
            val request = GetCredentialRequest.Builder().addCredentialOption(option).build()
            val response = CredentialManager.create(context).getCredential(context, request)
            val credential = response.credential
            if (credential is androidx.credentials.CustomCredential &&
                credential.type == GoogleIdTokenCredential.TYPE_GOOGLE_ID_TOKEN_CREDENTIAL
            ) {
                val google = GoogleIdTokenCredential.createFrom(credential.data)
                Account(
                    id = google.id,
                    displayName = google.displayName ?: google.givenName ?: google.id,
                    email = google.id
                )
            } else {
                Log.w(TAG, "Unexpected credential type: ${credential.type}")
                null
            }
        } catch (e: Exception) {
            Log.w(TAG, "Google sign-in failed or cancelled", e)
            null
        }
    }
}
