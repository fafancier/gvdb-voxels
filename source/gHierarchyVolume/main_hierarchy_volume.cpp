#include <iostream>
#include <gvdb.h>


int main(){
	
	
	
	VolumeGVDB	gvdb;
	// Initialize GVDB
	printf("Starting GVDB.\n");
	gvdb.SetDebug(true);
	gvdb.SetVerbose(true);		// enable/disable console output from gvdb
	gvdb.SetCudaDevice(GVDB_DEV_FIRST);
	gvdb.Initialize();
	gvdb.AddPath("../source/shared_assets/");
	gvdb.AddPath(ASSET_PATH);

	Vector3DF mem = cudaGetMemUsage();
	std::cout << "used " << mem.x / 1024.0 << " GBs; free " << mem.y/1024.0 << " GBs;  total " << mem.z / 1024.0 <<" GBs "<< std::endl;
	

	printf("Loading polygon model.\n");

	gvdb.getScene()->AddModel("lucy.obj", 1.0, 0, 0, 0);

	Vector3DF	m_pivot;

	m_pivot.Set(0.3f, 0.45f, 0.3f); // This is the center of the polygon model.

	printf("Configure.\n");
	gvdb.Configure(0, 0, 3, 3, 4);
	gvdb.SetChannelDefault(16, 16, 4);

	gvdb.DestroyChannels();
	gvdb.AddChannel(0, T_FLOAT, 1);
	float voxel_size = 0.2f;
	float part_size = 100.0;					// Part size = 100 mm (default)

	// Create a transform	
	Matrix4F xform, m;
	xform.Identity();

	// Complete poly-to-voxel transform: 
	//    X = S(partsize) S(1/voxelsize) Torigin R
	//  (remember, matrices are multiplied left-to-right but applied conceptually right-to-left)
	m.Scale(part_size, part_size, part_size);
	xform *= m;									// 4th) Apply part size 
	m.Scale(1 / part_size, 1 / part_size, 1 / part_size);
	xform *= m;									// 3rd) Apply voxel size (scale by inverse of this)
	m.Translate(m_pivot.x, m_pivot.y, m_pivot.z);	// 2nd) Move part so origin is at bottom corner 
	xform *= m;
	m.RotateZYX(Vector3DF(0, -10, 0));			// 1st) Rotate part about the geometric center
	xform *= m;

	// Set transform for rendering
	// Scale the GVDB grid by voxelsize to render the model in our desired world coordinates
	gvdb.SetTransform(Vector3DF(0, 0, 0), Vector3DF(part_size, part_size, part_size), Vector3DF(0, 0, 0), Vector3DF(0, 0, 0));

	// Poly-to-Voxels
	// Converts polygons-to-voxels using the GPU	
	Model* model = gvdb.getScene()->getModel(0);

	gvdb.SolidVoxelize(0, model, &xform, 1.0, 0.5);

	gvdb.Measure(true);



return 0;
}