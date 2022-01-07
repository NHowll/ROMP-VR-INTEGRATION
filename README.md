# ROMP-VR-INTEGRATION
ROMP, Monocular, One-stage, Regression, used to guide virtual avatars with OpenVR

Utilizing the ROMP repo for the model - https://github.com/Arthur151/ROMP
Driverside, I used terminal29's Simple OpenVR Driver Tutorial, as I'd never written anything like it before - https://github.com/terminal29/Simple-OpenVR-Driver-Tutorial

Pretty much just a hasty slapping together of projects, thanks to the amazing research done by the ROMP team

Tested on windows 10

Comment these two lines out in base.py if you, like me, have major issues installing pytorch3d on windows
> from visualization.visualization import Visualizer
> 
> self.visualizer = Visualizer(resolution=(512,512), result_img_dir=self.result_img_dir, with_renderer=True)
