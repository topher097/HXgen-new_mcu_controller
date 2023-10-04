from pathlib import Path
import os
import matplotlib.pyplot as plt
import sys

# Add the path of the main HX2.5 new microcontroller code to the path so that the pyEasyTransfer module can be imported
project_dir = Path(__file__).absolute().parent.parent
sys.path.append(os.path.join(project_dir))
python_EasyTransfer_dir = os.path.join(project_dir, "python_EasyTransfer")
sys.path.append(python_EasyTransfer_dir)

from analysis_main import Analysis, save_analysis, load_analysis
from analysis_logger import CustomLogger
import analysis_utils as utils
from analysis_plotter import Plotter




def run():
    project_dir = Path(__file__).absolute().parent.parent     # HXgen_new_mcu_controller
    parent_dir = Path(__file__).absolute().parent             # analysis   

    # Get the logger
    log_dir = os.path.join(parent_dir, "logs")
    logger = CustomLogger(log_dir=log_dir)
    
    # Main directories
    data_folder_name = "interest_data"
    
    # # First dataset
    # dataset_1_name = "single_vertical_piezo_10mm_wall_pyramids"
    # data_folder_path = os.path.join(parent_dir, data_folder_name, dataset_1_name)
    # dataset_1_filepaths = [os.path.join(data_folder_path, file) for file in os.listdir(data_folder_path) if file.endswith(".pkl")]
    
    # # second dataset
    # dataset_2_name = "single_vertical_piezo_15mm_wall"
    # data_folder_path = os.path.join(parent_dir, data_folder_name, dataset_2_name)
    # dataset_2_filepaths = [os.path.join(data_folder_path, file) for file in os.listdir(data_folder_path) if file.endswith(".pkl")]
    
    # Long pyramid tests dataset
    dataset_long_pyramid_name = "long_svp_10mm_pyramid"
    data_folder_path = os.path.join(parent_dir, data_folder_name, dataset_long_pyramid_name)
    dataset_long_pyramid_filepaths = [os.path.join(data_folder_path, file) for file in os.listdir(data_folder_path) if file.endswith(".pkl")]
    
    # # Long pyramid tests dataset
    # dataset_long_pyramid_name = "short_svp_15mm_control"
    # data_folder_path = os.path.join(parent_dir, data_folder_name, dataset_long_pyramid_name)
    # dataset_long_pyramid_filepaths = [os.path.join(data_folder_path, file) for file in os.listdir(data_folder_path) if file.endswith(".pkl")]
    
    # Create datasets dictionary
    # datasets = {dataset_1_name: dataset_1_filepaths,
    #             dataset_2_name: dataset_2_filepaths}
    datasets = {dataset_long_pyramid_name: dataset_long_pyramid_filepaths}
    
    # Create the save directory for analysis and plotter
    saved_analysis_dir = os.path.join(parent_dir, "saved_analysis")
    saved_plotting_dir = os.path.join(parent_dir, "saved_plots")
    
    
    # Run the analysis
    analysis = Analysis(datasets=datasets, log=logger.log)
    analysis.main()
    save_filename = save_analysis(analysis=analysis, analysis_save_dir=saved_analysis_dir, log=logger.log)
    print(save_filename)
    
    # Load the analysis
    #analysis_filename = "analysis_single_vertical_piezo_10mm_wall_pyramids_single_vertical_piezo_15mm_wall.pkl"
    #analysis_filename = "analysis_long_10mm_pyramid.pkl"
    #analysis = load_analysis(analysis_save_dir=saved_analysis_dir, analysis_filename=analysis_filename, log=logger.log)
    plotter = Plotter(analysis=analysis, save_plot_parent_dir=saved_plotting_dir, log=logger.log)
    plotter.plot()
    
    
if __name__ == "__main__":
    print("running analysis")
    # Run the analsis and plotter
    run()
    
    # Show the plots from the plotter
    plt.show()