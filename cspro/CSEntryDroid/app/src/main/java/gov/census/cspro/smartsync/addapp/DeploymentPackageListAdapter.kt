package gov.census.cspro.smartsync.addapp

import android.view.LayoutInflater
import android.view.ViewGroup
import androidx.databinding.DataBindingUtil
import androidx.recyclerview.widget.DiffUtil
import gov.census.cspro.commonui.DataBoundListAdapter
import gov.census.cspro.csentry.R
import gov.census.cspro.csentry.databinding.DeploymentPackageItemBinding

class DeploymentPackageListAdapter(private val onClick : (DeploymentPackage) -> Unit, private val showStatus : Boolean) : DataBoundListAdapter<DeploymentPackage, DeploymentPackageItemBinding>(

    diffCallback = object : DiffUtil.ItemCallback<DeploymentPackage>() {
        override fun areItemsTheSame(oldItem: DeploymentPackage, newItem: DeploymentPackage): Boolean {
            return oldItem == newItem
        }

        override fun areContentsTheSame(oldItem: DeploymentPackage, newItem: DeploymentPackage): Boolean {
            return oldItem.name == newItem.name
        }
    }
) {

    override fun createBinding(parent: ViewGroup): DeploymentPackageItemBinding {
        val binding : DeploymentPackageItemBinding = DataBindingUtil.inflate(
            LayoutInflater.from(parent.context),
            R.layout.deployment_package_item,
            parent,
            false
        )
        binding.showStatus = showStatus
        return binding
    }

    override fun bind(binding: DeploymentPackageItemBinding, item: DeploymentPackage) {
        binding.deploymentPackage = item
        binding.buttonUpdate.setOnClickListener {
            onClick(item)
        }
    }
}