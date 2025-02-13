package gov.census.cspro.smartsync.addapp

import android.content.Intent
import android.os.Bundle
import android.widget.Toast
import androidx.appcompat.app.AppCompatActivity
import androidx.databinding.DataBindingUtil
import androidx.lifecycle.ViewModelProvider
import androidx.lifecycle.lifecycleScope
import com.dropbox.core.android.Auth
import gov.census.cspro.commonui.runEngine
import gov.census.cspro.csentry.R
import gov.census.cspro.csentry.databinding.ActivityUpdateApplicationsBinding
import gov.census.cspro.csentry.ui.EntryActivity
import gov.census.cspro.engine.EngineInterface
import gov.census.cspro.engine.Messenger
import gov.census.cspro.engine.functions.AuthorizeDropboxFunction
import kotlinx.coroutines.launch

class UpdateApplicationsActivity : AppCompatActivity() {

    private lateinit var viewModel: UpdateApplicationViewModel

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        viewModel = ViewModelProvider(this).get(UpdateApplicationViewModel::class.java)

        val binding: ActivityUpdateApplicationsBinding = DataBindingUtil.setContentView(this, R.layout.activity_update_applications)
        binding.viewModel = viewModel
        binding.lifecycleOwner = this
        val adapter = DeploymentPackageListAdapter({ dp : DeploymentPackage -> updateApp(dp)}, false)
        binding.applist.adapter = adapter
        viewModel.getUpdatableApplications().observe(this, { updated ->
            adapter.submitList(updated)
        })
        viewModel.getCanceled().observe(this, { canceled -> if (canceled && !isFinishing) finish() })
    }

    override fun onStart() {
        super.onStart()
        loadUpdatableApps()
    }

    override fun onDestroy() {

        // Cancel pending updates
        EngineInterface.getInstance().onProgressDialogCancel()

        lifecycleScope.launch {
            runEngine { viewModel.cleanup() }
        }
        super.onDestroy()
    }

    override fun onResume() {
        super.onResume()
        if(AuthorizeDropboxFunction.isAuthenticating()){
            AuthorizeDropboxFunction.setAuthenticationComplete()
            var dbxCredential: String? = ""
            val credential = Auth.getDbxCredential()
            if (credential != null) {
                dbxCredential = credential.toString()
            }
            Messenger.getInstance().engineFunctionComplete(dbxCredential)
        }
    }

    @Deprecated("Deprecated in Java")
    override fun onActivityResult(requestCode: Int, resultCode: Int, intent: Intent?) {
        super.onActivityResult(requestCode, resultCode, intent)
    }

    private fun loadUpdatableApps()
    {
        if (!viewModel.loaded) {
            lifecycleScope.launch {
                runEngine {
                    viewModel.loadUpdatableApps()
                }
            }
        }
    }

    private fun updateApp(deploymentPackage: DeploymentPackage)
    {
        val context = this

        lifecycleScope.launch {

            val result = runEngine { viewModel.updateApp(deploymentPackage) }

            if (result)
                Toast.makeText(context, String.format(getString(R.string.add_app_update_success), deploymentPackage.name), Toast.LENGTH_LONG).show()

            finish()
        }
    }

}
