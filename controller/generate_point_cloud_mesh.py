import numpy as np
import open3d as o3d
import os
from colour import Color
import time

from data_classes import Vibrometer
from utils import load_vibrometer_object, get_point_object, get_nearest_freq_index


"""Calculate the z_array given the z_array and t_frame """
def calculate_zt(z_array: np.ndarray, t_frame: float, period: float=1.0) -> np.ndarray:
    # Scale the z_array between 0 and 1 and then separate the real and imaginary parts. Use those parts to calcualte a new array with the function of z_real*sin(t_frame + z_imag)
    z_real = np.real(z_array)
    z_imag = np.imag(z_array)
    z_real = (z_real - np.min(z_real)) / (np.max(z_real) - np.min(z_real))
    z_imag = (z_imag - np.min(z_imag)) / (np.max(z_imag) - np.min(z_imag))    
    return z_real * np.cos(period*(t_frame - z_imag))



"""Create the range of mesh objects to create an animation of the mesh changing"""
class AnimationGenerator:
    def __init__(self, vibrometer: Vibrometer, point_number: int=1, target_frequency: float=1000.0, animation_length_ms: int=4000, animation_steps: int=100):
        self.vibrometer = vibrometer
        self.freq_array = vibrometer.freq
        self.point_number = point_number
        self.target_frequency = target_frequency
        self.animation_length_ms = animation_length_ms
        self.animation_steps = animation_steps
        
        # Set the point dimension scales
        self.xy_scale = 1
        self.z_scale = 0.1
        self.z_min = 0
        self.z_max = 1
        
        # Create point color gradient, lowest z value will be blue, highest z value will be red
        self.gradient_steps = 300
        color_grad_list = list(Color("blue").range_to(Color("red"), self.gradient_steps))
        self.color_gradient = np.asarray(list(map(lambda x: x.rgb, color_grad_list)))
        
        # Create the essential arrays
        self.t = np.linspace(0, 2*np.pi, self.animation_steps, endpoint=True)
        self.xy_array = self.create_xy_array()
        self.zt_array = None
        self.rgb_array = None
        self.norm_array = self.create_norm_array()
        #print(self.xy_array)
        
        # Create the list for animation frames
        self.animation_mesh_frames = None
        self.animation_point_frames = None
            
    """Main function to generate a list of frames"""
    def generate_frames(self, target_frequency: float, point_number: int, generate_mesh_frames: bool=True, generate_point_frames: bool=True) -> None:
        # Recalculate the zt_array and the rgb_array
        self.zt_array = self.create_zt_array(target_frequency=target_frequency)
        self.point_number = point_number
        
        # First frame, z0 is the max value since cosine function. Use this to calculate the rgb_array
        z0 = self.zt_array[:, 0]        # First frame, z(t=0)
        self.z_min = np.min(z0)
        self.z_max = np.max(z0)
        self.rgb_array = self.create_rgb_array(z0=z0)
        xyzrgbnorm0_array = np.concatenate((self.xy_array, np.reshape(z0, (-1, 1)), self.rgb_array, self.norm_array), axis=1)
        frame0_mesh = PointCloudMeshGenerator(xyzrgbnorm0_array).mesh
        
        # Quarter frame, zpi/2 is the min value since cosine function
        zhalfpi = self.zt_array[:, int(self.animation_steps/4)]        # First frame, z(t=pi/2)
        xyzrgbnormhalfpi_array = np.concatenate((self.xy_array, np.reshape(zhalfpi, (-1, 1)), self.rgb_array, self.norm_array), axis=1)
        framehalfpi_mesh = PointCloudMeshGenerator(xyzrgbnormhalfpi_array).mesh
        
        # Point cloud for single point at the given point number
        self.point_index_of_array = self.vibrometer.measured_points.index(self.point_number)
        self.xy_point = self.xy_array[self.point_index_of_array]  # Current point number xy
        self.z_point = zhalfpi[self.point_index_of_array]         # Current point number z, use the zhalfpi array
        self.rgb_point = np.array([255, 255, 255])     # White
        self.norm_point = np.array([0, 0, 1])          # Norm positive z
        xyzrgbnorm_point = np.concatenate((self.xy_point, np.array([self.z_point]), self.rgb_point, self.norm_point))
        xyzrgbnorm_point = np.vstack((xyzrgbnorm_point, xyzrgbnorm_point))
        # pcd_point = o3d.geometry.PointCloud()
        # pcd_point.points = o3d.utility.Vector3dVector(xyzrgbnorm_point[:, 0:3])
        # pcd_point.colors = o3d.utility.Vector3dVector(xyzrgbnorm_point[:, 3:6])
        # pcd_point.normals = o3d.utility.Vector3dVector(xyzrgbnorm_point[:, 6:9])
        
        # Iterate over frame num to create the mesh and point animation frames
        if generate_mesh_frames:
            self.animation_mesh_frames = list()
            rgbnorm_array = np.concatenate((self.rgb_array, self.norm_array), axis=1)
            for frame_num in range(self.animation_steps):
                # Generate the TriangleMesh object for the frame
                frame_mesh = self.transform_mesh(framehalfpi_mesh, frame_num, rgbnorm_array)
                # Append mesh object to list
                self.animation_mesh_frames.append(frame_mesh)
        
        if generate_point_frames:
            self.animation_point_frames = list()
            point_rgbnorm_array = xyzrgbnorm_point[:, 3:9]
            for frame_num in range(self.animation_steps):
                # Generate the point cloud object for the frame
                frame_point = self.transform_point(frame_num, point_rgbnorm_array)
                # Append point cloud object to list
                self.animation_point_frames.append(frame_point)
            
    """Create the xy portion of the xyzrgb array"""
    def create_xy_array(self) -> np.ndarray:
        xy_array = np.zeros(shape=(len(self.vibrometer.measured_points), 2))
        for i in range(len(self.vibrometer.measured_points)):
            xy = self.vibrometer.geometry.pointXYZ[i, :2]
            xy_array[i] = xy
        # Scale the xy_array between 0 and 1
        xy_array = self.xy_scale * (xy_array - np.min(xy_array))/(np.max(xy_array) - np.min(xy_array))
        return xy_array
    
    """Create the z(t) array to feed into the xyzrgb array"""
    def create_zt_array(self, target_frequency: float) -> np.ndarray:
        # First calculate the z_array for given frequency
        z_array = np.zeros(shape=(len(self.vibrometer.measured_points), 1), dtype=complex)
        index = get_nearest_freq_index(self.freq_array, target_frequency)
        for i in range(len(self.vibrometer.measured_points)):
            point_num = self.vibrometer.measured_points[i]
            point = get_point_object(self.vibrometer.points, point_num)
            frf_data_for_target_freq = point.frf.data[index]
            z_array[i] = frf_data_for_target_freq
        # Iterate through the t_frames and calculate the zt_array
        zt_array = np.zeros((len(self.vibrometer.measured_points), self.animation_steps))
        for frame_num in range(self.animation_steps):
            zt_array[:, frame_num] = calculate_zt(z_array, self.t[frame_num]).flatten()
        return zt_array * self.z_scale
        
    """Calculate the rgb array for the current frame based on the z value and range min and max"""
    def create_rgb_array(self, z0: np.ndarray) -> np.ndarray:
        rgb_array = np.zeros(shape=(len(self.vibrometer.measured_points), 3))
        # Note: z0 is the z_array at t=0 (since sine function, t0 is max amplitude)
        z_min = np.min(z0)
        z_max = np.max(z0)
        for i in range(len(self.vibrometer.measured_points)):
            z_value = z0[i]
            percent_of_range = (z_value - z_min) / (z_max - z_min)
            color_index = int(percent_of_range * (self.gradient_steps-1))
            point_color = self.color_gradient[color_index]
            rgb_array[i] = point_color
        return rgb_array
    
    """Create the normal vectors for each point, normal is strictly in z direction"""
    def create_norm_array(self) -> np.ndarray:
        norm_array = np.zeros(shape=(len(self.vibrometer.measured_points), 3))
        norm_array[:, 2] = 1
        return norm_array
    
    """Transform the initial mesh to animate the mesh over time"""
    def transform_mesh(self, mesh: o3d.geometry.TriangleMesh, frame_num: int, rgbnorm_array: np.ndarray) -> o3d.geometry.TriangleMesh:
        # Create the xyz transformation array
        z_transform = self.zt_array[:, frame_num]
        xyz_transform = np.concatenate((self.xy_array, np.reshape(z_transform, (-1, 1))), axis=1)
        # Set the vertices of the mesh using the transformation
        triangles = mesh.triangles
        vertices = o3d.utility.Vector3dVector(xyz_transform)
        new_mesh = o3d.geometry.TriangleMesh(vertices, triangles)
        new_mesh.vertex_colors = o3d.utility.Vector3dVector(rgbnorm_array[:, :3])
        new_mesh.vertex_normals = o3d.utility.Vector3dVector(rgbnorm_array[:, 3:])
        return new_mesh

    def transform_point(self, frame_num: int, rgbnorm_array: np.ndarray) -> o3d.geometry.PointCloud:
        pcd = o3d.geometry.PointCloud()
        z = self.zt_array[self.point_index_of_array, frame_num]
        xyz_transform = np.concatenate((self.xy_point, np.array([z])))
        xyz_transform = np.vstack((xyz_transform, xyz_transform))
        pcd.points = o3d.utility.Vector3dVector(xyz_transform)
        pcd.colors = o3d.utility.Vector3dVector(rgbnorm_array[:, :3])
        pcd.normals = o3d.utility.Vector3dVector(rgbnorm_array[:, 3:])
        return pcd
            

"""Generate the point cloud mesh for the given frequency"""
class PointCloudMeshGenerator:
    def __init__(self, xyzrgbnorm_array: np.ndarray, calc_mesh: bool = True):        
        # Define the point cloud and triangle mesh object
        self.pcd = None
        self.mesh = None
        
        # Fill in the numpy array, then convert to point cloud object
        self.pcd = self.convert_np_to_pc(xyzrgbnorm_array)
        if calc_mesh:
            self.mesh = self.create_mesh(self.pcd)
             
    def convert_np_to_pc(self, xyzrgbnorm_array) -> o3d.geometry.PointCloud:
        pcd = o3d.geometry.PointCloud()
        pcd.points = o3d.utility.Vector3dVector(xyzrgbnorm_array[:,:3])
        pcd.colors = o3d.utility.Vector3dVector(xyzrgbnorm_array[:,3:6])
        pcd.normals = o3d.utility.Vector3dVector(xyzrgbnorm_array[:,6:9])
        #o3d.visualization.draw_geometries([pcd])
        return pcd
        
    def calc_ball_radius(self, pcd) -> float:
        # Calculate the radius of the ball based on the point cloud
        # This is done by calculating the median distance between points and multiplying by a factor
        m = 2
        dists = np.asarray(pcd.compute_nearest_neighbor_distance())
        dists = dists[abs(dists - np.mean(dists)) < m * np.std(dists)]
        ball_radius = np.min(dists) * 1.1
        return ball_radius
        
    def create_mesh(self, pcd) -> o3d.geometry.TriangleMesh:
        # Calculate the point distances and the radius of ball
        radius = self.calc_ball_radius(pcd)
        
        # Create the mesh and clean up
        mesh = o3d.geometry.TriangleMesh.create_from_point_cloud_ball_pivoting(pcd, o3d.utility.DoubleVector([radius]))
        mesh.remove_degenerate_triangles()
        mesh.remove_duplicated_triangles()
        mesh.remove_duplicated_vertices()
        mesh.remove_non_manifold_edges()   
        
        return mesh
    
    

def visualize_non_blocking(vis, pcds):
    for pcd in pcds:
        vis.update_geometry(pcd)
    vis.poll_events()
    vis.update_renderer()

if __name__=="__main__":
    vibrometer = load_vibrometer_object(prefix="channel_top_11112022")
    output_dir = os.path.join(os.getcwd(), 'output')
    output_file_name = 'mesh.xyz'
    #xyzfile_gen = GenerateXYZFile(vibrometer=vibrometer, output_dir=output_dir, file_name=output_file_name)
    
    #generator = PointCloudMeshGenerator(vibrometer=vibrometer)
    animation_gen = AnimationGenerator(vibrometer=vibrometer)
    animation_gen.generate_frames(target_frequency=1000.0)
    print(np.max(animation_gen.xy_array))
    print(np.max(animation_gen.zt_array))
    
    frames = animation_gen.animation_frames[0::5]
    print(frames)
    o3d.visualization.draw_geometries(frames).get_view_control().mesh_show_back_face = True
    exit()
    
    vis = o3d.visualization.Visualizer()
    vis.create_window()
    start_time = time.time()*1000
    pcds = animation_gen.animation_frames
    n_pcd = len(pcds)
    added = [False] * n_pcd

    dt = 5000 # ms
    curr_sec = time.time()*1000 - start_time
    prev_sec = curr_sec - dt
    curr_frame = 0
    curr_camera = None
    
    while True:
        curr_sec = time.time()*1000 - start_time
        if curr_sec - prev_sec >= dt:
            print(f"Frame: {curr_frame}")
            temp = vis.get_view_control().convert_to_pinhole_camera_parameters()
            print(temp.extrinsic)
            prev_sec = curr_sec
            # Remove last mesh
            if curr_frame != 0:
                vis.remove_geometry(pcds[curr_frame-1])
            # Add new mesh
            vis.add_geometry(pcds[curr_frame])
            # Reinstate camera view
            if curr_frame != 0:
                print("moving camera view")
                vis.get_view_control().convert_from_pinhole_camera_parameters(temp)
                print(f"new camera: {temp.extrinsic}")
           
            # Limit frame number 
            curr_frame = (curr_frame + 1) % n_pcd
            
        visualize_non_blocking(vis, pcds)
        
    
    
    
    