#include "sp_segmenter/SIFTpooling.h"

//#define LAB_ONLY

#ifdef LAB_ONLY

int main(int argc, char** argv)
{
    std::string in_path("/home/chi/UW_RGBD/filtered_pcd/");
    std::string out_path("uw_tmp/");
//    std::string shot_path("JHU_kmeans_dict/");
    std::string shot_path("BB_new_dict/");
    std::string sift_path("JHU_sift_dict09/");
    //std::string sift_path("JHU_densesift_dict/");
    std::string fpfh_path("UW_fpfh_dict/");
    pcl::console::parse_argument(argc, argv, "--p", in_path);
    pcl::console::parse_argument(argc, argv, "--o", out_path);
    boost::filesystem::create_directories(out_path);
    
    int c1 = 0, c2 = -1;
    
    pcl::console::parse_argument(argc, argv, "--c1", c1);
    pcl::console::parse_argument(argc, argv, "--c2", c2);
    
/***************************************************************************************************************/

    float radius = 0.03;
    float down_ss = 0.003;
    float ratio = 0.1;
    //float sigma = 0.9;
    pcl::console::parse_argument(argc, argv, "--rd", radius);
    pcl::console::parse_argument(argc, argv, "--rt", ratio);
    pcl::console::parse_argument(argc, argv, "--ss", down_ss);
    //pcl::console::parse_argument(argc, argv, "--sigma", sigma);
    std::cerr << "Ratio: " << ratio << std::endl;
    std::cerr << "Downsample: " << down_ss << std::endl;

    std::vector<cv::SiftFeatureDetector*> sift_det_vec;
    for( float sigma = 0.8 ; sigma <= 1.0 ; sigma += 0.1 )
    {	
        cv::SiftFeatureDetector *sift_det = new cv::SiftFeatureDetector(
        	0, // nFeatures
        	4, // nOctaveLayers
        	-10000, // contrastThreshold 
        	100000, //edgeThreshold
        	sigma//sigma
        	);
        sift_det_vec.push_back(sift_det);	
    }
    
    int fea_dim = -1;
    Hier_Pooler hie_producer(radius);
    hie_producer.LoadDict_L0(shot_path, "200", "200");
    hie_producer.setRatio(ratio);
   
    std::vector< boost::shared_ptr<Pooler_L0> > sift_pooler_set(1+1);
    for( size_t i = 1 ; i < sift_pooler_set.size() ; i++ )
    {
        boost::shared_ptr<Pooler_L0> cur_pooler(new Pooler_L0(-1));
        sift_pooler_set[i] = cur_pooler;
    }
    //sift_pooler_set[1]->LoadSeedsPool(sift_path+"dict_sift_L0_25.cvmat");
    //sift_pooler_set[2]->LoadSeedsPool(sift_path+"dict_sift_L0_50.cvmat");
    //sift_pooler_set[3]->LoadSeedsPool(sift_path+"dict_sift_L0_100.cvmat");
    //sift_pooler_set[4]->LoadSeedsPool(sift_path+"dict_sift_L0_200.cvmat");
    sift_pooler_set[1]->LoadSeedsPool(sift_path+"dict_sift_L0_100.cvmat"); 

    std::vector< boost::shared_ptr<Pooler_L0> > lab_pooler_set(5+1);
    for( size_t i = 1 ; i < lab_pooler_set.size() ; i++ )
    {
        boost::shared_ptr<Pooler_L0> cur_pooler(new Pooler_L0);
        cur_pooler->setHSIPoolingParams(i);
        lab_pooler_set[i] = cur_pooler;
    }

    std::vector< boost::shared_ptr<Pooler_L0> > fpfh_pooler_set(1+1);
    for( size_t i = 1 ; i < fpfh_pooler_set.size() ; i++ )
    {
        boost::shared_ptr<Pooler_L0> cur_pooler(new Pooler_L0(-1));
        fpfh_pooler_set[i] = cur_pooler;
    }
    //fpfh_pooler_set[1]->LoadSeedsPool(sift_path+"dict_fpfh_L0_25.cvmat");
    //fpfh_pooler_set[2]->LoadSeedsPool(sift_path+"dict_fpfh_L0_50.cvmat");
    //fpfh_pooler_set[3]->LoadSeedsPool(sift_path+"dict_fpfh_L0_100.cvmat");
    //fpfh_pooler_set[4]->LoadSeedsPool(sift_path+"dict_fpfh_L0_200.cvmat");
    fpfh_pooler_set[1]->LoadSeedsPool(fpfh_path+"dict_fpfh_L0_100.cvmat");

    //Pooler_L0 genericPooler(-1);
    //genericPooler.LoadSeedsPool(sift_path+"dict_sift_L0_200.cvmat");
    
/***************************************************************************************************************/  
    for( int i = c1 ; i <= c2 ; i++ )
    {
        ObjectSet train_objects, test_objects;
        
//        readUWInstWithImg(in_path, train_objects, test_objects, i, i, 1);
        readUWInst(in_path, train_objects, test_objects, i, i, 1);
        std::cerr << "Loading Completed... " << std::endl;
        
        int train_num = train_objects[0].size();
        std::cerr << "Train " << i << " --- " << train_num << std::endl;
        
        if( train_num > 0 )
        {
            std::vector< sparseVec> final_train;
            #pragma omp parallel for schedule(dynamic, 1)
            for( int j = 0 ; j < train_num ; j++ )
            {
//              pcl::PointCloud<PointT>::Ptr mycloud = train_objects[0][j].cloud;
//            	pcl::PointCloud<NormalT>::Ptr mycloud_normals(new pcl::PointCloud<NormalT>());
//            	computeNormals(mycloud, mycloud_normals, radius);
//		MulInfoT tmp_data = convertPCD(mycloud, mycloud_normals);
                
                MulInfoT tmp_data = convertPCD(train_objects[0][j].cloud, train_objects[0][j].cloud_normals);
//                tmp_data.img = train_objects[0][j].img;
//                tmp_data.map2d = train_objects[0][j].map2d;
//                tmp_data._3d2d = train_objects[0][j]._3d2d;
              
//                cv::Mat cur_atlas = cv::Mat::zeros(tmp_data.img.rows, tmp_data.img.cols, CV_32SC1);
//                for(int k = 0 ; k < tmp_data._3d2d.rows ; k++ )
//                    cur_atlas.at<int>(tmp_data._3d2d.at<int>(k, 1), tmp_data._3d2d.at<int>(k, 0) ) = 1;
//                
//                std::vector<cv::Mat> cur_final_vec = SIFTPooling(tmp_data, sift_det_vec, sift_ext, hie_producer, sift_pooler_set, cur_atlas, 1);
		cv::Mat ext_fea;
		pcl::VoxelGrid<PointT> sor;
                sor.setInputCloud(tmp_data.cloud);
                sor.setLeafSize(down_ss, down_ss, down_ss);
                sor.filter(*tmp_data.down_cloud);
                PreCloud(tmp_data, -1, false);
                std::vector<cv::Mat> main_fea = hie_producer.getHierFea(tmp_data, 0);

                //std::vector<cv::Mat> cur_final_vec = SIFTPooling_new(tmp_data, main_fea, sift_ext, sift_pooler_set, cur_atlas, 1);

                cv::Mat lab_fea = multiPool(lab_pooler_set, tmp_data, main_fea);
//                cv::Mat fpfh_fea = multiFPFHPool(fpfh_pooler_set, tmp_data, main_fea, radius);
//		    cv::hconcat(fpfh_fea, lab_fea, ext_fea);
		
		cv::Mat cur_final = lab_fea;
//		cv::hconcat(cur_final_vec[1], ext_fea, cur_final);
                //cv::Mat cur_final = cur_final_vec[1];
                if( fea_dim > 0 && cur_final.cols != fea_dim )
                {
                    std::cerr << "Error: fea_dim > 0 && cur_final.cols != fea_dim   " << fea_dim << " " << cur_final.cols << std::endl;
                    exit(0);
                }
                else if( fea_dim < 0 )
		{
		#pragma omp critical
		{
                    fea_dim = cur_final.cols;
		    std::cerr << "Fea Dim: " << fea_dim << std::endl;
		}
		}	
                std::vector< sparseVec> this_sparse;
                sparseCvMat(cur_final, this_sparse);
                #pragma omp critical
                {
                    final_train.push_back(this_sparse[0]);
                }
                
            }
            std::stringstream ss;
            ss << i+1;
        
            saveCvMatSparse(out_path + "train_"+ss.str()+"_L0.smat", final_train, fea_dim);
            final_train.clear();
        }
        train_objects.clear();
        test_objects.clear();
        
        
//        readUWInstWithImg(in_path, train_objects, test_objects, i, i, 5);
        readUWInst(in_path, train_objects, test_objects, i, i, 5);
        int test_num = test_objects[0].size();
        std::cerr << "Test " << i << " --- " << test_num << std::endl;
        if( test_num > 0 )
        {
            std::vector< sparseVec> final_test;
            #pragma omp parallel for schedule(dynamic, 1)
            for( int j = 0 ; j < test_num ; j++ )
            {
//                pcl::PointCloud<PointT>::Ptr mycloud = test_objects[0][j].cloud;
//            	pcl::PointCloud<NormalT>::Ptr mycloud_normals(new pcl::PointCloud<NormalT>());
//            	computeNormals(mycloud, mycloud_normals, radius);
//		MulInfoT tmp_data = convertPCD(mycloud, mycloud_normals);

                MulInfoT tmp_data = convertPCD(test_objects[0][j].cloud, test_objects[0][j].cloud_normals);
//                tmp_data.img = test_objects[0][j].img;
//                tmp_data.map2d = test_objects[0][j].map2d;
//                tmp_data._3d2d = test_objects[0][j]._3d2d;
              
//                cv::Mat cur_atlas = cv::Mat::zeros(tmp_data.img.rows, tmp_data.img.cols, CV_32SC1);
//                for(int k = 0 ; k < tmp_data._3d2d.rows ; k++ )
//                    cur_atlas.at<int>(tmp_data._3d2d.at<int>(k, 1), tmp_data._3d2d.at<int>(k, 0) ) = 1;
//                
//                std::vector<cv::Mat> cur_final_vec = SIFTPooling(tmp_data, sift_det_vec, sift_ext, hie_producer, sift_pooler_set, cur_atlas, 1);
		cv::Mat ext_fea;
		pcl::VoxelGrid<PointT> sor;
                sor.setInputCloud(tmp_data.cloud);
                sor.setLeafSize(down_ss, down_ss, down_ss);
                sor.filter(*tmp_data.down_cloud);
                PreCloud(tmp_data, -1, false);
                std::vector<cv::Mat> main_fea = hie_producer.getHierFea(tmp_data, 0);

                //std::vector<cv::Mat> cur_final_vec = SIFTPooling_new(tmp_data, main_fea, sift_ext, sift_pooler_set, cur_atlas, 1);

                cv::Mat lab_fea = multiPool(lab_pooler_set, tmp_data, main_fea);
//		    cv::Mat fpfh_fea = multiFPFHPool(fpfh_pooler_set, tmp_data, main_fea, radius);
//		    cv::hconcat(fpfh_fea, lab_fea, ext_fea);
		
		cv::Mat cur_final = lab_fea;
//		cv::hconcat(cur_final_vec[1], ext_fea, cur_final);
                //cv::Mat cur_final = cur_final_vec[1];
                if( fea_dim > 0 && cur_final.cols != fea_dim )
                {
                    std::cerr << "Error: fea_dim > 0 && cur_final.cols != fea_dim   " << fea_dim << " " << cur_final.cols << std::endl;
                    exit(0);
                }
                else if( fea_dim < 0 )
		{
		#pragma omp critical
		{
                    fea_dim = cur_final.cols;
		    std::cerr << "Fea Dim: " << fea_dim << std::endl;
		}
		}	
                std::vector< sparseVec> this_sparse;
                sparseCvMat(cur_final, this_sparse);
                #pragma omp critical
                {
                    final_test.push_back(this_sparse[0]);
                }
                
            }
            std::stringstream ss;
            ss << i+1;
        
            saveCvMatSparse(out_path + "test_"+ss.str()+"_L0.smat", final_test, fea_dim);
            final_test.clear();
        }
        test_objects.clear();
    
    }
    
    std::cerr << "Fea_dim: " << fea_dim << std::endl;
    //*/ 
    return 1;
} 

#else

int main(int argc, char** argv)
{
    std::string in_path("/home/chi/UW_RGBD/rgbd-dataset/");
    std::string in_path1("/home/chi/UW_RGBD/filtered_pcd/");
    std::string out_path("uw_final/");
    std::string shot_path("UW_shot_dict/");
    std::string sift_path("UW_new_sift_dict/");
    std::string fpfh_path("UW_fpfh_dict/");
//    pcl::console::parse_argument(argc, argv, "--p", in_path);
    pcl::console::parse_argument(argc, argv, "--o", out_path);
    boost::filesystem::create_directories(out_path);
    
    int c1 = 0, c2 = -1;
    pcl::console::parse_argument(argc, argv, "--c1", c1);
    pcl::console::parse_argument(argc, argv, "--c2", c2);
/***************************************************************************************************************/

    float radius = 0.02;
    float down_ss = 0.003;
    float ratio = 0.1;
    //float sigma = 0.9;
    pcl::console::parse_argument(argc, argv, "--rd", radius);
    pcl::console::parse_argument(argc, argv, "--rt", ratio);
    pcl::console::parse_argument(argc, argv, "--ss", down_ss);
    //pcl::console::parse_argument(argc, argv, "--sigma", sigma);
    std::cerr << "Ratio: " << ratio << std::endl;
    std::cerr << "Downsample: " << down_ss << std::endl;

    std::vector<cv::SiftFeatureDetector*> sift_det_vec;
    for( float sigma = 0.7 ; sigma <= 1.61 ; sigma += 0.1 )
    {	
        cv::SiftFeatureDetector *sift_det = new cv::SiftFeatureDetector(
        	0, // nFeatures
        	4, // nOctaveLayers
        	-10000, // contrastThreshold 
        	100000, //edgeThreshold
        	sigma//sigma
        	);
        sift_det_vec.push_back(sift_det);	
    }
    cv::SiftDescriptorExtractor * sift_ext = new cv::SiftDescriptorExtractor();
    
    int fea_dim = -1;
    Hier_Pooler hie_producer(radius);
    std::vector<int> shot_len = hie_producer.LoadDict_L0(shot_path, "200", "200");
    hie_producer.setRatio(ratio);
   
    int sift_len = 0;
    std::vector< boost::shared_ptr<Pooler_L0> > sift_pooler_set(1+1);
    for( size_t i = 1 ; i < sift_pooler_set.size() ; i++ )
    {
        boost::shared_ptr<Pooler_L0> cur_pooler(new Pooler_L0(-1));
        sift_pooler_set[i] = cur_pooler;
    }
    //sift_pooler_set[1]->LoadSeedsPool(sift_path+"dict_sift_L0_25.cvmat");
    //sift_pooler_set[2]->LoadSeedsPool(sift_path+"dict_sift_L0_50.cvmat");
    //sift_pooler_set[3]->LoadSeedsPool(sift_path+"dict_sift_L0_100.cvmat");
    //sift_pooler_set[4]->LoadSeedsPool(sift_path+"dict_sift_L0_200.cvmat");
    sift_len += sift_pooler_set[1]->LoadSeedsPool(sift_path+"dict_sift_L0_400.cvmat"); 
    sift_len = sift_len*(shot_len[0]+shot_len[1]);
    
    std::vector< boost::shared_ptr<Pooler_L0> > lab_pooler_set(5+1);
    for( size_t i = 1 ; i < lab_pooler_set.size() ; i++ )
    {
        boost::shared_ptr<Pooler_L0> cur_pooler(new Pooler_L0);
        cur_pooler->setHSIPoolingParams(i);
        lab_pooler_set[i] = cur_pooler;
    }

    std::vector< boost::shared_ptr<Pooler_L0> > fpfh_pooler_set(1+1);
    for( size_t i = 1 ; i < fpfh_pooler_set.size() ; i++ )
    {
        boost::shared_ptr<Pooler_L0> cur_pooler(new Pooler_L0(-1));
        fpfh_pooler_set[i] = cur_pooler;
    }
    //fpfh_pooler_set[1]->LoadSeedsPool(sift_path+"dict_fpfh_L0_25.cvmat");
    //fpfh_pooler_set[2]->LoadSeedsPool(sift_path+"dict_fpfh_L0_50.cvmat");
    //fpfh_pooler_set[3]->LoadSeedsPool(sift_path+"dict_fpfh_L0_100.cvmat");
    //fpfh_pooler_set[4]->LoadSeedsPool(sift_path+"dict_fpfh_L0_200.cvmat");
    fpfh_pooler_set[1]->LoadSeedsPool(fpfh_path+"dict_fpfh_L0_400.cvmat");

    //Pooler_L0 genericPooler(-1);
    //genericPooler.LoadSeedsPool(sift_path+"dict_sift_L0_200.cvmat");
    
/***************************************************************************************************************/  
    for( int i = c1 ; i <= c2 ; i++ )
    {
        ObjectSet train_img, train_cloud, test_img, test_cloud;
        
//        readUWInstWithImg(in_path, train_objects, test_objects, i, i, 1);
//        readUWInst("/home/chi/UW_RGBD/filtered_pcd/", train_objects, test_objects, i, i, 1);
        readUWInstTwo(in_path, in_path1, train_img, train_cloud, test_img, test_cloud, i, i, 5, 5);
        if( train_img[0].size() != train_cloud[0].size() || test_img[0].size() != test_cloud[0].size() )
        {
            std::cerr << "train_img[0].size() != train_cloud[0].size() || test_img[0].size() != test_cloud[0].size()" << std::endl;
            std::cerr << train_img[0].size() << " " << train_cloud[0].size() << std::endl;
            std::cerr << test_img[0].size() << " " << test_cloud[0].size() << std::endl;
            exit(0);
        }
//        continue;
        
        std::cerr << "Loading Completed... " << std::endl;
        
        int train_num = train_img[0].size();
        std::cerr << "Train " << i << " --- " << train_num << std::endl;
        if( train_num > 0 )
        {
            std::vector< sparseVec> final_train;
            #pragma omp parallel for schedule(dynamic, 1)
            for( int j = 0 ; j < train_num ; j++ )
            {
                
                cv::Mat sift_fea;
                {
                    pcl::PointCloud<PointT>::Ptr mycloud = train_img[0][j].cloud;
                    pcl::PointCloud<NormalT>::Ptr mycloud_normals(new pcl::PointCloud<NormalT>());
                    computeNormals(mycloud, mycloud_normals, radius);
                
                    MulInfoT tmp_data = convertPCD(mycloud, mycloud_normals);
                
                    tmp_data.img = getFullImage(mycloud);

                    cv::Mat cur_atlas = cv::Mat::ones(tmp_data.img.rows, tmp_data.img.cols, CV_32SC1)*(-1);
                    cv::Mat map2d = cv::Mat::ones(tmp_data.img.rows, tmp_data.img.cols, CV_32SC1)*(-1);

                    int width = mycloud->width;
                    int pt_count = 0;

                    for( pcl::PointCloud<PointT>::iterator it = mycloud->begin() ; it < mycloud->end() ; it++, pt_count++ )
                    {
                        if( pcl_isfinite(it->z) == true )
                        {
                            int img_x = pt_count % width;
                            int img_y = pt_count / width;
                            map2d.at<int>(img_y, img_x) = pt_count;
                            cur_atlas.at<int>(img_y, img_x) = 1;
                        }
                    }

                    tmp_data.map2d = map2d;
                    std::vector<cv::Mat> cur_final_vec = SIFTPooling(tmp_data, sift_det_vec, sift_ext, hie_producer, sift_pooler_set, cur_atlas, 1);
                    
                    if( cur_final_vec[1].empty() == true )
                    {
                        std::cerr << "HaHa!" << std::endl;
                        cur_final_vec[1] = cv::Mat::zeros(1, sift_len, CV_32FC1);
                    }
                    sift_fea = cur_final_vec[1];
                }
                
                cv::Mat fpfh_lab_fea;
                {
                    pcl::PointCloud<PointT>::Ptr mycloud = train_cloud[0][j].cloud;
                    pcl::PointCloud<NormalT>::Ptr mycloud_normals(new pcl::PointCloud<NormalT>());
                    computeNormals(mycloud, mycloud_normals, radius);
                
                    MulInfoT tmp_data = convertPCD(mycloud, mycloud_normals);
                
                    pcl::VoxelGrid<PointT> sor;
                    sor.setInputCloud(tmp_data.cloud);
                    sor.setLeafSize(down_ss, down_ss, down_ss);
                    sor.filter(*tmp_data.down_cloud);
                    PreCloud(tmp_data, -1, false);
                    std::vector<cv::Mat> main_fea = hie_producer.getHierFea(tmp_data, 0);

                    cv::Mat lab_fea = multiPool(lab_pooler_set, tmp_data, main_fea);
                    cv::Mat fpfh_fea = multiFPFHPool(fpfh_pooler_set, tmp_data, main_fea, radius);
                    cv::hconcat(fpfh_fea, lab_fea, fpfh_lab_fea);
                
                }
                cv::Mat cur_final;
                cv::hconcat(sift_fea, fpfh_lab_fea, cur_final);
                
                
                if( fea_dim > 0 && cur_final.cols != fea_dim )
                {
                    std::cerr << "Error: fea_dim > 0 && cur_final.cols != fea_dim   " << fea_dim << " " << cur_final.cols << std::endl;
                    exit(0);
                }
                else if( fea_dim < 0 )
		{
		#pragma omp critical
		{
                    fea_dim = cur_final.cols;
		    std::cerr << "Fea Dim: " << fea_dim << std::endl;
		}
		}	
                std::vector< sparseVec> this_sparse;
                sparseCvMat(cur_final, this_sparse);
                #pragma omp critical
                {
                    final_train.push_back(this_sparse[0]);
                }
                
            }
            std::stringstream ss;
            ss << i+1;
        
            saveCvMatSparse(out_path + "train_"+ss.str()+"_L0.smat", final_train, fea_dim);
            final_train.clear();
        }
        train_img.clear();
        train_cloud.clear();
        
        
        int test_num = test_img[0].size();
        std::cerr << "Test " << i << " --- " << test_num << std::endl;
        if( test_num > 0 )
        {
            std::vector< sparseVec> final_test;
            #pragma omp parallel for schedule(dynamic, 1)
            for( int j = 0 ; j < test_num ; j++ )
            {
                cv::Mat sift_fea;
                {
                    pcl::PointCloud<PointT>::Ptr mycloud = test_img[0][j].cloud;
                    pcl::PointCloud<NormalT>::Ptr mycloud_normals(new pcl::PointCloud<NormalT>());
                    computeNormals(mycloud, mycloud_normals, radius);
                
                    MulInfoT tmp_data = convertPCD(mycloud, mycloud_normals);
                
                    tmp_data.img = getFullImage(mycloud);

                    cv::Mat cur_atlas = cv::Mat::ones(tmp_data.img.rows, tmp_data.img.cols, CV_32SC1)*(-1);
                    cv::Mat map2d = cv::Mat::ones(tmp_data.img.rows, tmp_data.img.cols, CV_32SC1)*(-1);

                    int width = mycloud->width;
                    int pt_count = 0;

                    for( pcl::PointCloud<PointT>::iterator it = mycloud->begin() ; it < mycloud->end() ; it++, pt_count++ )
                    {
                        if( pcl_isfinite(it->z) == true )
                        {
                            int img_x = pt_count % width;
                            int img_y = pt_count / width;
                            map2d.at<int>(img_y, img_x) = pt_count;
                            cur_atlas.at<int>(img_y, img_x) = 1;
                        }
                    }

                    tmp_data.map2d = map2d;
                    std::vector<cv::Mat> cur_final_vec = SIFTPooling(tmp_data, sift_det_vec, sift_ext, hie_producer, sift_pooler_set, cur_atlas, 1);
                    
                    if( cur_final_vec[1].empty() == true )
                    {
                        std::cerr << "HaHa!" << std::endl;
                        cur_final_vec[1] = cv::Mat::zeros(1, sift_len, CV_32FC1);
                    }
                    sift_fea = cur_final_vec[1];
                }
                
                cv::Mat fpfh_lab_fea;
                {
                    pcl::PointCloud<PointT>::Ptr mycloud = test_cloud[0][j].cloud;
                    pcl::PointCloud<NormalT>::Ptr mycloud_normals(new pcl::PointCloud<NormalT>());
                    computeNormals(mycloud, mycloud_normals, radius);
                
                    MulInfoT tmp_data = convertPCD(mycloud, mycloud_normals);
                
                    pcl::VoxelGrid<PointT> sor;
                    sor.setInputCloud(tmp_data.cloud);
                    sor.setLeafSize(down_ss, down_ss, down_ss);
                    sor.filter(*tmp_data.down_cloud);
                    PreCloud(tmp_data, -1, false);
                    std::vector<cv::Mat> main_fea = hie_producer.getHierFea(tmp_data, 0);

                    cv::Mat lab_fea = multiPool(lab_pooler_set, tmp_data, main_fea);
                    cv::Mat fpfh_fea = multiFPFHPool(fpfh_pooler_set, tmp_data, main_fea, radius);
                    cv::hconcat(fpfh_fea, lab_fea, fpfh_lab_fea);
                
                }
                cv::Mat cur_final;
                cv::hconcat(sift_fea, fpfh_lab_fea, cur_final);
                
                
                if( fea_dim > 0 && cur_final.cols != fea_dim )
                {
                    std::cerr << "Error: fea_dim > 0 && cur_final.cols != fea_dim   " << fea_dim << " " << cur_final.cols << std::endl;
                    exit(0);
                }
                else if( fea_dim < 0 )
		{
		#pragma omp critical
		{
                    fea_dim = cur_final.cols;
		    std::cerr << "Fea Dim: " << fea_dim << std::endl;
		}
		}	
                std::vector< sparseVec> this_sparse;
                sparseCvMat(cur_final, this_sparse);
                #pragma omp critical
                {
                    final_test.push_back(this_sparse[0]);
                }
                
            }
            std::stringstream ss;
            ss << i+1;
        
            saveCvMatSparse(out_path + "test_"+ss.str()+"_L0.smat", final_test, fea_dim);
            final_test.clear();
        }
        test_img.clear();
        test_cloud.clear();
    }
    
    std::cerr << "Fea_dim: " << fea_dim << std::endl;
    //*/ 
    return 1;
} 

#endif