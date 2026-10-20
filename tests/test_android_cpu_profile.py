import csv,io,json,pathlib,sys,unittest
from unittest.mock import patch
sys.path.insert(0,str(pathlib.Path(__file__).resolve().parents[1]/'tools'))
import analyze_android_cpu_profile as profile

class CpuProfileJoinTests(unittest.TestCase):
    def fixture(self):
        rows=[];frames=[]
        for phase in profile.VIEWS:
            for tick in range(59,82):
                serial=len(rows)+1
                r={'serial':serial,'tick':serial,'phase':phase,'view_ticks':tick,'x':1,'y':2,'z':3,'yaw':0,'pitch':0,'cpu_stage_status':'valid'}
                for s in profile.CPU:r[s+'_cpu_ns']=0;r[s+'_calls']=0
                r.update(skin_cpu_ns=3_000_000,player_skin_cpu_ns=2_000_000,character_skin_cpu_ns=1_000_000,
                         skin_calls=2,player_skin_calls=1,character_skin_calls=1,whole_frame_cpu_ns=8_000_000,whole_frame_calls=1)
                rows.append(r);frames.append({'submission':serial,'tick':serial,'wholeFrameCpuNs':8_000_000,'gpuNs':12_000_000})
        return rows,{'motionEvidence':{'completedRtFrames':frames}}
    def run_join(self,rows,report):
        out=io.StringIO();w=csv.DictWriter(out,fieldnames=rows[0]);w.writeheader();w.writerows(rows)
        # Isolate this CPU join; the real runner always calls the existing strict
        # hardware/presentation analyzer first. Unit fixtures are not device runs.
        with patch.object(profile,'analyze_bytes',return_value={'fixture':True}):
            return profile.analyze(json.dumps(report).encode(),out.getvalue())
    def test_joins_all_rows_keeps_zero_work_and_separates_gpu(self):
        rows,j=self.fixture();r=self.run_join(rows,j)
        self.assertEqual(r['joinedFrames'],92)
        self.assertEqual(r['allRouteCpu']['skin']['all']['medianMs'],3)
        self.assertIsNone(r['allRouteCpu']['pose_copy']['active'])
        self.assertEqual(r['views']['lower-sweep']['gpu']['medianMs'],12)
    def test_rejects_reordered_or_wrong_tick_or_missing_owner(self):
        for kind in ('serial','tick','row'):
            rows,j=self.fixture()
            if kind=='row':rows.pop()
            else:rows[5][kind]=rows[4][kind]
            with self.assertRaises(profile.AnalysisError):self.run_join(rows,j)
    def test_rejects_unavailable_or_borrowed_cpu_and_bad_aggregate(self):
        for key,value in [('cpu_stage_status','pending'),('whole_frame_cpu_ns',9),('skin_cpu_ns',2),('pose_copy_cpu_ns',-1)]:
            rows,j=self.fixture();rows[0][key]=value
            with self.assertRaises(profile.AnalysisError):self.run_join(rows,j)
    def test_rejects_moving_or_underfilled_held_view(self):
        for kind in ('moving','warmup'):
            rows,j=self.fixture()
            for r in rows[:23]:
                if kind=='warmup':r['view_ticks']=1
            if kind=='moving':rows[8]['x']=1.01
            with self.assertRaises(profile.AnalysisError):self.run_join(rows,j)

if __name__=='__main__':unittest.main()
