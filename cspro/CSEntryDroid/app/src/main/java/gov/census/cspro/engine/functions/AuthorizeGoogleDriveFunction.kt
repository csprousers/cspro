package gov.census.cspro.engine.functions

import android.app.Activity
import android.net.Uri
import androidx.activity.result.ActivityResult
import gov.census.cspro.engine.Messenger
import net.openid.appauth.AuthorizationException
import net.openid.appauth.AuthorizationRequest
import net.openid.appauth.AuthorizationResponse
import net.openid.appauth.AuthorizationService
import net.openid.appauth.AuthorizationServiceConfiguration
import net.openid.appauth.ResponseTypeValues

class AuthorizeGoogleDriveFunction(private val oauth2Endpoint: String, private val tokenEndpoint: String,
                                   private val clientId: String, private val scope: String?,
                                   private val additionalParameters: Map<String, String>) : EngineFunction {
    private lateinit var authService: AuthorizationService

    companion object {
        private const val REDIRECT_URI = "gov.census.cspro.csentry:/oauth2"
    }

    override fun runEngineFunction(activity: Activity) {
        val authServiceConfig = AuthorizationServiceConfiguration(
            Uri.parse(oauth2Endpoint),
            Uri.parse(tokenEndpoint)
        )

        val authRequestBuilder = AuthorizationRequest.Builder(
            authServiceConfig,
            clientId,
            ResponseTypeValues.CODE,
            Uri.parse(REDIRECT_URI)
        )

        if( scope != null ) {
            authRequestBuilder.setScope(scope)
        }

        authRequestBuilder.setAdditionalParameters(additionalParameters)

        val authRequest = authRequestBuilder.build()
        authService = AuthorizationService(activity)
        val authIntent = authService.getAuthorizationRequestIntent(authRequest)

        Messenger.getInstance().startActivityForResultFromEngineFunction(activity, { result -> onResult(result) }, authIntent)
    }

    private fun onResult(result: ActivityResult) {
        try {
            if( result.data == null ) {
                throw Exception()
            }

            val authError = AuthorizationException.fromIntent(result.data)
            if( authError != null ) {
                throw Exception(authError.error)
            }

            val authResponse = AuthorizationResponse.fromIntent(result.data!!) ?: throw Exception()
            val tokenExchangeRequest = authResponse.createTokenExchangeRequest()

            authService.performTokenRequest(tokenExchangeRequest) { response, exception ->
                if( exception != null ) {
                    returnException(exception)
                }
                else if( response == null ) {
                    returnException(null)
                }
                else {
                    returnToken(response.jsonSerializeString())
                }
            }
        }
        catch( ex: Exception ) {
            returnException(ex)
        }
    }

    private fun returnException(ex: Exception?) {
        Messenger.getInstance().engineFunctionComplete(if( ex?.message == null ) { null } else { "e:" + ex.message })
    }

    private fun returnToken(jsonText: String) {
        Messenger.getInstance().engineFunctionComplete("t:" + jsonText)
    }
}
