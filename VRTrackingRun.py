import sys 
whether_set_yml = ['configs_yml' in input_arg for input_arg in sys.argv]
if sum(whether_set_yml)==0:
    default_webcam_configs_yml = "--configs_yml=configs/vrOutputs.yml"
    print('No configs_yml is set, set it to the default {}'.format(default_webcam_configs_yml))
    sys.argv.append(default_webcam_configs_yml)
from romp.predict.base_predictor import *
from romp.predict.video import get_tracked_ids
if args().tracker == 'norfair':
    from norfair import Detection, Tracker
    import norfair
else:
    from romp.lib.tracking.tracker import Tracker
from romp.lib.utils.temporal_optimization import create_OneEuroFilter, temporal_optimize_result

class Webcam_processor(Predictor):
    def __init__(self, **kwargs):
        super(Webcam_processor, self).__init__(**kwargs)
        if self.character == 'nvxia':
            assert os.path.exists(os.path.join('model_data','characters','nvxia')), \
                'Current released version does not support other characters, like Nvxia.'
            from romp.lib.models.nvxia import create_nvxia_model
            self.character_model = create_nvxia_model(self.nvxia_model_path)

    def webcam_run_local(self, video_file_path=None):
        '''
        24.4 FPS of forward prop. on 1070Ti
        '''
        from romp.lib.utils.demo_utils import OpenCVCapture
        capture = OpenCVCapture(video_file_path, show=False)
        print('Initialization is down')
        frame_id = 0
        if self.visulize_platform == 'integrated':
            from romp.lib.visualization.open3d_visualizer import Open3d_visualizer
            visualizer = Open3d_visualizer(multi_mode=not args().show_largest_person_only)
        elif self.visulize_platform == 'vrOutput':
            print("going")

        if self.make_tracking:
            if args().tracker == 'norfair':
                if args().tracking_target=='centers':
                    tracker = Tracker(distance_function=euclidean_distance, distance_threshold=80)
                elif args().tracking_target=='keypoints':
                    tracker = Tracker(distance_function=keypoints_distance, distance_threshold=60)
            else:
                tracker = Tracker()

        if self.temporal_optimization:
            filter_dict = {}
            subjects_motion_sequences = {}

        # Warm-up
        for i in range(10):
            self.single_image_forward(np.zeros((512,512,3)).astype(np.uint8))
        counter = Time_counter(thresh=1)

        while True:
            start_time_perframe = time.time()
            frame = capture.read()
            if frame is None:
                continue

            frame_id+=1
            counter.start()
            with torch.no_grad():
                outputs = self.single_image_forward(frame)
            counter.count()
            #counter.fps()

            if outputs is not None and outputs['detection_flag']:
                reorganize_idx = outputs['reorganize_idx'].cpu().numpy()
                results = self.reorganize_results(outputs, [frame_id for _ in range(len(reorganize_idx))], reorganize_idx)

                if args().show_largest_person_only or self.visulize_platform == 'blender':
                    max_id = np.argmax(np.array([result['cam'][0] for result in results[frame_id]]))
                    results[frame_id] = [results[frame_id][max_id]]
                    tracked_ids = np.array([0])
                
                elif args().make_tracking:
                    if args().tracker == 'norfair':
                        if args().tracking_target=='centers':
                            detections = [Detection(points=result['cam'][[2,1]]*args().input_size) for result in results[frame_id]]
                        elif args().tracking_target=='keypoints':
                            detections = [Detection(points=result['pj2d_org']) for result in results[frame_id]]
                        # norfair takes at least 6 frames for initialization without generating any tracking results.
                        if frame_id==1:
                            for _ in range(8):
                                tracked_objects = tracker.update(detections=detections)
                        tracked_objects = tracker.update(detections=detections)
                        if len(tracked_objects)==0:
                            continue
                        tracked_ids = get_tracked_ids(detections, tracked_objects)
                        #norfair.draw_tracked_objects(frame, tracked_objects,id_size=5,id_thickness=2)
                    else:
                        tracked_ids = tracker.update(results[frame_id])
                    if len(tracked_ids)==0 or len(tracked_ids) > len(results[frame_id]):
                        continue
                else:
                    tracked_ids = np.arange(len(results[frame_id]))

                cv2.imshow('Input', frame[:,:,::-1])
                cv2.waitKey(1)
                
                if self.temporal_optimization:
                    for sid, tid in enumerate(tracked_ids):
                        if tid not in filter_dict:
                            filter_dict[tid] = create_OneEuroFilter(args().smooth_coeff)
                            subjects_motion_sequences[tid] = {}
                        results[frame_id][sid] = temporal_optimize_result(results[frame_id][sid], filter_dict[tid])
                        subjects_motion_sequences[tid][frame_id] = results[frame_id][sid]
                
                cams = np.array([result['cam'] for result in results[frame_id]])
                # to elimate the y-axis offset
                cams[:,2] -= 0.26
                trans = np.array([convert_cam_to_3d_trans(cam) for cam in cams])
                poses = np.array([result['poses'] for result in results[frame_id]])
                betas = np.array([result['betas'] for result in results[frame_id]])
                kp3ds = np.array([result['j3d_smpl24'] for result in results[frame_id]])
                verts = np.array([result['verts'] for result in results[frame_id]])
                
                if self.visulize_platform == 'vrOutput':
                    kp3ds = kp3ds[0].tolist()
                    #print("sending")
                    #sender.send([kp3ds[2], kp3ds[0], kp3ds[5], frame_id])
                    #IO
                    #fileTime = time.time()
                    open("C:/temp/File.txt", "w").write(str((kp3ds[0])[0])+"\n"+str((kp3ds[0])[1])+"\n"+str((kp3ds[0])[2])+"\n"+str((kp3ds[8])[0])+"\n"+str((kp3ds[8])[1])+"\n"+str((kp3ds[8])[2])+"\n"+str((kp3ds[7])[0])+"\n"+str((kp3ds[7])[1])+"\n"+str((kp3ds[7])[2])+"\n"+str(frame_id))
                    #fileDuration = time.time() - fileTime
                    #print("File Write Duration: %.2f s." % fileDuration)
                    #
                elif self.visulize_platform == 'integrated':
                    if self.character == 'nvxia':
                        verts = self.character_model(poses)['verts'].numpy()
                    trans_largest = trans[0] if self.add_trans else None
                    # please make sure the (x, y, z) of visualized verts are all in range (-1, 1)
                    # print(verts_largest.max(0), verts_largest.min(0))
                    kp3ds = kp3ds[0].tolist()
                    print([kp3ds[0],frame_id])#Pelvis
                    print([kp3ds[8],frame_id])#Right Ankle
                    print([kp3ds[7],frame_id])#Left Ankle
                    visualizer.run(verts[0], trans=trans_largest)

def euclidean_distance(detection, tracked_object):
    return np.linalg.norm(detection.points - tracked_object.estimate)

def main():
    with ConfigContext(parse_args(sys.argv[1:])) as args_set:
        print('Loading the configurations from {}'.format(args_set.configs_yml))
        processor = Webcam_processor(args_set=args_set)
        print('Running vr outputs')
        processor.webcam_run_local()

if __name__ == '__main__':
    main()